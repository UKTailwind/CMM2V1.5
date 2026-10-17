/***************************************************************************

CMM2 MMBasic
USBDrive.c

A USB flash drive as C:, through TinyUSB's MSC host class.

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
/*
 * USBDrive.c -- a USB flash drive as C:, physical drive 1 to FatFs (mmc_stm32.c dispatches by drive).
 *
 * This is the PicoMite's USB MSC layer (misc/SDCard.c), as it is but for the platform: the microsecond clock
 * (GetuSec), the delays (HAL_Delay), and the callbacks' messages, which wait in USBMessage() until tuh_task() has
 * returned, as the keyboard's do.  The PicoMite also calls TinyUSB's interrupt handler from its wait loops, for an
 * interrupt that may not fire; here OTG_FS's does, and calling the handler from the main thread too would race it.
 */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "ff.h"
#include "diskio.h"
#include "tusb.h"
#include "class/msc/msc_host.h"

extern char filepath[];

/*--------------------------------------------------------------------------

   USB Mass Storage Class - Internal Functions (pdrv 1 = C: drive)

---------------------------------------------------------------------------*/

/* USB MSC state tracking */
static volatile bool msc_mounted = false;
static volatile bool msc_transfer_complete = false;
static volatile DRESULT msc_transfer_result = RES_ERROR;
static uint8_t msc_dev_addr = 0;
volatile BYTE USBDriveStat = STA_NOINIT | STA_NODISK;  /* the drive's FatFs status (the PicoMite's is in FileIO.c) */

#define USB_MSC_MAX_SECTORS_PER_XFER 127 /* TinyUSB host transfer length is uint16_t bytes */
#define USB_MSC_LATENCY_MONITOR 0
#define USB_MSC_FORCE_READ_SINGLE_SECTOR 0
#define USB_MSC_READ_PATTERN_MONITOR 0
#define USB_MSC_READAHEAD_ENABLE 0
#define USB_MSC_READAHEAD_SECTORS 8
#define USB_MSC_SEQ2_PREFETCH_ENABLE 0
#define USB_MSC_SEQ2_PREFETCH_MIN_STREAK 2
#define USB_MSC_CHAIN_RETRIES 4
#define USB_MSC_WAIT_READY_TIMEOUT_MS 1000
#define USB_MSC_SUBMIT_RETRY_BACKOFF_US 200
#define USB_MSC_SUBMIT_MONITOR 0
#define USB_MSC_HOTPATH_IN_RAM 0

#if USB_MSC_HOTPATH_IN_RAM
#define USB_MSC_HOT_FUNC(name) __not_in_flash_func(name)
#else
#define USB_MSC_HOT_FUNC(name) name
#endif

typedef enum
{
	MSC_CHAIN_NONE = 0,
	MSC_CHAIN_READ,
	MSC_CHAIN_WRITE
} msc_chain_op_t;

typedef struct
{
	volatile bool active;
	volatile bool complete;
	volatile bool submit_pending;
	volatile DRESULT result;
	msc_chain_op_t op;
	BYTE *read_buff;
	const BYTE *write_buff;
	LBA_t sector;
	UINT remaining;
	uint32_t submits_issued;
} msc_chain_state_t;

static msc_chain_state_t msc_chain_state = {0};

#if USB_MSC_SEQ2_PREFETCH_ENABLE
typedef struct
{
	bool valid;
	LBA_t sector;
	LBA_t last_single_sector;
	bool have_last_single;
	uint8_t sequential_single_streak;
	BYTE prefetched_sector[512];
	BYTE pair_buffer[1024];
} msc_seq2_prefetch_t;

static msc_seq2_prefetch_t msc_seq2_prefetch = {0};

static inline void msc_seq2_prefetch_invalidate(void)
{
	msc_seq2_prefetch.valid = false;
	msc_seq2_prefetch.have_last_single = false;
	msc_seq2_prefetch.sequential_single_streak = 0;
}
#endif

#if USB_MSC_SUBMIT_MONITOR
typedef struct
{
	uint32_t read_ops;
	uint32_t write_ops;
	uint32_t submits_ok;
	uint32_t submits_fail;
	uint32_t initial_submit_fail;
	uint32_t mid_submit_fail;
	uint32_t wait_ready_fail;
	uint32_t chain_timeout;
} msc_submit_stats_t;

static msc_submit_stats_t msc_submit_stats = {0};

static void msc_submit_report(bool force)
{
	uint32_t total_ops = msc_submit_stats.read_ops + msc_submit_stats.write_ops;
	if (total_ops == 0)
		return;
	if (!force && total_ops < 256)
		return;

	MMPrintString("USB MSC submit stats: rd=");
	PInt((int)msc_submit_stats.read_ops);
	MMPrintString(" wr=");
	PInt((int)msc_submit_stats.write_ops);
	MMPrintString(" ok=");
	PInt((int)msc_submit_stats.submits_ok);
	MMPrintString(" fail=");
	PInt((int)msc_submit_stats.submits_fail);
	MMPrintString(" fail0=");
	PInt((int)msc_submit_stats.initial_submit_fail);
	MMPrintString(" failN=");
	PInt((int)msc_submit_stats.mid_submit_fail);
	MMPrintString(" wait=");
	PInt((int)msc_submit_stats.wait_ready_fail);
	MMPrintString(" tmo=");
	PInt((int)msc_submit_stats.chain_timeout);
	MMPrintString("\r\n");

	msc_submit_stats.read_ops = 0;
	msc_submit_stats.write_ops = 0;
	msc_submit_stats.submits_ok = 0;
	msc_submit_stats.submits_fail = 0;
	msc_submit_stats.initial_submit_fail = 0;
	msc_submit_stats.mid_submit_fail = 0;
	msc_submit_stats.wait_ready_fail = 0;
	msc_submit_stats.chain_timeout = 0;
}
#endif

#if USB_MSC_READAHEAD_ENABLE
typedef struct
{
	bool valid;
	LBA_t start_sector;
	UINT sector_count;
	BYTE data[USB_MSC_READAHEAD_SECTORS * 512];
} msc_readahead_cache_t;

static msc_readahead_cache_t msc_readahead_cache = {0};

static inline void msc_readahead_invalidate(void)
{
	msc_readahead_cache.valid = false;
	msc_readahead_cache.sector_count = 0;
}
#endif

#if USB_MSC_READ_PATTERN_MONITOR
typedef struct
{
	uint32_t calls;
	uint32_t single_calls;
	uint32_t multi_calls;
	uint32_t sequential_calls;
	uint32_t total_sectors;
	LBA_t last_end_sector;
	bool have_last;
} msc_read_pattern_stats_t;

static msc_read_pattern_stats_t msc_read_pattern = {0};

static void msc_read_pattern_record(LBA_t sector, UINT count)
{
	msc_read_pattern.calls++;
	msc_read_pattern.total_sectors += count;
	if (count == 1)
		msc_read_pattern.single_calls++;
	else
		msc_read_pattern.multi_calls++;

	if (msc_read_pattern.have_last && sector == msc_read_pattern.last_end_sector)
		msc_read_pattern.sequential_calls++;

	msc_read_pattern.last_end_sector = sector + count;
	msc_read_pattern.have_last = true;
}

static void msc_read_pattern_report(bool force)
{
	if (msc_read_pattern.calls == 0)
		return;
	if (!force && msc_read_pattern.calls < 256)
		return;

	MMPrintString("USB MSC read pattern: calls=");
	PInt((int)msc_read_pattern.calls);
	MMPrintString(" single=");
	PInt((int)msc_read_pattern.single_calls);
	MMPrintString(" multi=");
	PInt((int)msc_read_pattern.multi_calls);
	MMPrintString(" seq=");
	PInt((int)msc_read_pattern.sequential_calls);
	MMPrintString(" sectors=");
	PInt((int)msc_read_pattern.total_sectors);
	MMPrintString(" avg_count_x100=");
	PInt((int)((msc_read_pattern.total_sectors * 100u) / msc_read_pattern.calls));
	MMPrintString("\r\n");

	msc_read_pattern.calls = 0;
	msc_read_pattern.single_calls = 0;
	msc_read_pattern.multi_calls = 0;
	msc_read_pattern.sequential_calls = 0;
	msc_read_pattern.total_sectors = 0;
	msc_read_pattern.have_last = false;
}
#endif

#if USB_MSC_LATENCY_MONITOR
typedef struct
{
	uint32_t samples;
	uint64_t sum_us;
	uint32_t min_us;
	uint32_t max_us;
	uint32_t around_1ms;
	uint32_t under_500us;
	uint32_t over_2ms;
} msc_latency_stats_t;

static volatile uint64_t msc_read_submit_us = 0;
static msc_latency_stats_t msc_latency_window = {0, 0, 0xFFFFFFFFu, 0, 0, 0, 0};

static inline void msc_latency_record(uint32_t us)
{
	msc_latency_window.samples++;
	msc_latency_window.sum_us += us;
	if (us < msc_latency_window.min_us)
		msc_latency_window.min_us = us;
	if (us > msc_latency_window.max_us)
		msc_latency_window.max_us = us;
	if (us >= 900 && us <= 1300)
		msc_latency_window.around_1ms++;
	if (us < 500)
		msc_latency_window.under_500us++;
	if (us > 2000)
		msc_latency_window.over_2ms++;
}

static void msc_latency_report(bool force)
{
	if (msc_latency_window.samples == 0)
		return;
	if (!force && msc_latency_window.samples < 128)
		return;

	MMPrintString("USB MSC read cb latency us: n=");
	PInt((int)msc_latency_window.samples);
	MMPrintString(" avg=");
	PInt((int)(msc_latency_window.sum_us / msc_latency_window.samples));
	MMPrintString(" min=");
	PInt((int)msc_latency_window.min_us);
	MMPrintString(" max=");
	PInt((int)msc_latency_window.max_us);
	MMPrintString(" ~1ms=");
	PInt((int)msc_latency_window.around_1ms);
	MMPrintString(" <500us=");
	PInt((int)msc_latency_window.under_500us);
	MMPrintString(" >2ms=");
	PInt((int)msc_latency_window.over_2ms);
	MMPrintString("\r\n");

	msc_latency_window.samples = 0;
	msc_latency_window.sum_us = 0;
	msc_latency_window.min_us = 0xFFFFFFFFu;
	msc_latency_window.max_us = 0;
	msc_latency_window.around_1ms = 0;
	msc_latency_window.under_500us = 0;
	msc_latency_window.over_2ms = 0;
}
#endif

static bool USB_MSC_HOT_FUNC(msc_complete_cb)(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data);

static bool USB_MSC_HOT_FUNC(msc_chain_submit_next)(void)
{
	if (!msc_chain_state.active || msc_chain_state.remaining == 0)
		return true;

	uint16_t chunk = (msc_chain_state.remaining > USB_MSC_MAX_SECTORS_PER_XFER) ? USB_MSC_MAX_SECTORS_PER_XFER : (uint16_t)msc_chain_state.remaining;
	bool submitted = false;

	if (msc_chain_state.op == MSC_CHAIN_READ)
	{
#if USB_MSC_FORCE_READ_SINGLE_SECTOR
		chunk = 1;
#endif
		submitted = tuh_msc_read10(msc_dev_addr, 0, msc_chain_state.read_buff, msc_chain_state.sector, chunk, msc_complete_cb, 0);
#if USB_MSC_LATENCY_MONITOR
		if (submitted)
			msc_read_submit_us = (uint64_t)GetuSec();
#endif
	}
	else if (msc_chain_state.op == MSC_CHAIN_WRITE)
		submitted = tuh_msc_write10(msc_dev_addr, 0, msc_chain_state.write_buff, msc_chain_state.sector, chunk, msc_complete_cb, 0);

	if (!submitted)
		return false;

	msc_chain_state.submits_issued++;

	if (msc_chain_state.op == MSC_CHAIN_READ)
		msc_chain_state.read_buff += (uint32_t)chunk * 512;
	else
		msc_chain_state.write_buff += (uint32_t)chunk * 512;

	msc_chain_state.sector += chunk;
	msc_chain_state.remaining -= chunk;
	return true;
}

static uint32_t msc_chain_timeout_ms(UINT sectors, uint32_t timeout_per_chunk_ms)
{
	uint32_t chunks = (sectors + USB_MSC_MAX_SECTORS_PER_XFER - 1) / USB_MSC_MAX_SECTORS_PER_XFER;
	uint64_t total = (uint64_t)chunks * timeout_per_chunk_ms;
	if (total == 0)
		total = timeout_per_chunk_ms;
	if (total > 0xFFFFFFFFULL)
		total = 0xFFFFFFFFULL;
	return (uint32_t)total;
}

static DRESULT USB_MSC_HOT_FUNC(msc_wait_chain_complete)(uint32_t timeout_ms)
{
	uint64_t deadline = (uint64_t)GetuSec() + (uint64_t)timeout_ms * 1000;
	while (!msc_chain_state.complete)
	{
		if (msc_chain_state.active && msc_chain_state.submit_pending)
		{
			if (msc_chain_submit_next())
			{
				msc_chain_state.submit_pending = false;
#if USB_MSC_SUBMIT_MONITOR
				msc_submit_stats.submits_ok++;
#endif
			}
			else
			{
#if USB_MSC_SUBMIT_MONITOR
				msc_submit_stats.submits_fail++;
				if (msc_chain_state.submits_issued == 0)
					msc_submit_stats.initial_submit_fail++;
				else
					msc_submit_stats.mid_submit_fail++;
#endif
				uSec(USB_MSC_SUBMIT_RETRY_BACKOFF_US);
			}
		}

		tuh_task();
		if ((uint64_t)GetuSec() >= deadline)
		{
			msc_chain_state.active = false;
			msc_chain_state.complete = true;
			msc_chain_state.submit_pending = false;
			msc_chain_state.result = RES_ERROR;
#if USB_MSC_SUBMIT_MONITOR
			msc_submit_stats.chain_timeout++;
#endif
			break;
		}
	}
	return msc_chain_state.result;
}

/* Completion callback shared by read10 and write10 */
static bool USB_MSC_HOT_FUNC(msc_complete_cb)(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data)
{
	(void)dev_addr;
	msc_transfer_result = (cb_data->csw->status == 0) ? RES_OK : RES_ERROR;
	msc_transfer_complete = true;

#if USB_MSC_LATENCY_MONITOR
	if (msc_chain_state.active && msc_chain_state.op == MSC_CHAIN_READ && msc_read_submit_us)
	{
		uint64_t now = (uint64_t)GetuSec();
		uint32_t delta = (now >= msc_read_submit_us) ? (uint32_t)(now - msc_read_submit_us) : 0;
		msc_latency_record(delta);
		msc_read_submit_us = 0;
	}
#endif

	if (!msc_chain_state.active)
		return true;

	if (msc_transfer_result != RES_OK)
	{
		msc_chain_state.result = msc_transfer_result;
		msc_chain_state.active = false;
		msc_chain_state.complete = true;
		return true;
	}

	if (msc_chain_state.remaining == 0)
	{
		msc_chain_state.result = RES_OK;
		msc_chain_state.active = false;
		msc_chain_state.submit_pending = false;
		msc_chain_state.complete = true;
		return true;
	}

	if (msc_chain_submit_next())
	{
		msc_chain_state.submit_pending = false;
#if USB_MSC_SUBMIT_MONITOR
		msc_submit_stats.submits_ok++;
#endif
	}
	else
	{
		msc_chain_state.submit_pending = true;
#if USB_MSC_SUBMIT_MONITOR
		msc_submit_stats.submits_fail++;
		msc_submit_stats.mid_submit_fail++;
#endif
	}

	return true;
}

/* Wait for the MSC interface to be idle and ready for a new command */
static bool USB_MSC_HOT_FUNC(msc_wait_ready)(uint32_t timeout_ms)
{
	uint64_t deadline = (uint64_t)GetuSec() + (uint64_t)timeout_ms * 1000;
	while (!tuh_msc_ready(msc_dev_addr))
	{
		tuh_task();
		if ((uint64_t)GetuSec() >= deadline)
			return false;
	}
	return true;
}

/* Wait for an async MSC transfer to complete, pumping tuh_task() */
static DRESULT msc_wait_complete(uint32_t timeout_ms)
{
	uint64_t deadline = (uint64_t)GetuSec() + (uint64_t)timeout_ms * 1000;
	while (!msc_transfer_complete)
	{
		tuh_task();
		if ((uint64_t)GetuSec() >= deadline)
			return RES_ERROR;
	}
	return msc_transfer_result;
}

/* TinyUSB callbacks - called from tuh_task() context */
void USBMessage(const char *m);   /* printed once tuh_task() has returned (USBKeyboard.c) */

void tuh_msc_mount_cb(uint8_t dev_addr)
{
	msc_dev_addr = dev_addr;
	msc_mounted = true;
	USBDriveStat &= ~STA_NODISK; /* device present; STA_NOINIT cleared by disk_initialize() */
	USBMessage("USB Flash Drive Connected\r\n> ");
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
	if (dev_addr == msc_dev_addr)
	{
		msc_mounted = false;
		msc_dev_addr = 0;
#if USB_MSC_SEQ2_PREFETCH_ENABLE
		msc_seq2_prefetch_invalidate();
#endif
#if USB_MSC_READAHEAD_ENABLE
		msc_readahead_invalidate();
#endif
		USBDriveStat |= (STA_NOINIT | STA_NODISK);
		USBMessage("USB Flash Drive Disconnected\r\n> ");
		if (filepath[0] == 'C') /* the current drive has gone: back to A:, as for the SD card */
		{
			strcpy(filepath, "A:/");
			f_chdrive("A:");
		}
	}
}

/* Return true if a USB MSC device is currently mounted */
bool usb_msc_is_mounted(void)
{
	return msc_mounted;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS usb_disk_status(void)
{
	/* Refresh device-present status from TinyUSB */
	if (msc_dev_addr && tuh_msc_mounted(msc_dev_addr))
		USBDriveStat &= ~STA_NODISK;
	else
		USBDriveStat |= (STA_NOINIT | STA_NODISK);

	return USBDriveStat;
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS usb_disk_initialize(void)
{
	/* Give TinyUSB a chance to enumerate */
	for (int i = 0; i < 50 && !msc_mounted; i++)
	{
		tuh_task();
		HAL_Delay(10);
	}

	if (msc_mounted && msc_dev_addr && tuh_msc_mounted(msc_dev_addr))
	{
		/* Send TEST_UNIT_READY until the drive's media is ready */
		for (int i = 0; i < 10; i++)
		{
			if (!msc_wait_ready(1000))
			{
				USBDriveStat |= STA_NOINIT;
				return USBDriveStat;
			}
			msc_transfer_complete = false;
			msc_transfer_result = RES_ERROR;
			if (tuh_msc_test_unit_ready(msc_dev_addr, 0, msc_complete_cb, 0))
			{
				if (msc_wait_complete(3000) == RES_OK)
					break; /* drive is ready */
			}
			HAL_Delay(100); /* give the drive time and retry */
		}

		/* Verify block size matches FatFS sector size (512) */
		uint32_t blk_size = tuh_msc_get_block_size(msc_dev_addr, 0);
		if (blk_size != 512)
		{
			if (!CurrentLinePtr)
			{
				MMPrintString("USB drive block size ");
				PInt(blk_size);
				MMPrintString(" not supported (need 512)\r\n> ");
			}
			USBDriveStat |= STA_NOINIT;
			return USBDriveStat;
		}
		USBDriveStat &= ~(STA_NOINIT | STA_NODISK);
	}
	else
	{
		USBDriveStat |= STA_NOINIT;
	}

	return USBDriveStat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s) - raw USB transfer (no caching)                        */
/*-----------------------------------------------------------------------*/

static DRESULT usb_disk_read_chain(BYTE *buff, LBA_t sector, UINT count)
{
	DRESULT res = RES_ERROR;
	int retries = USB_MSC_CHAIN_RETRIES;

#if USB_MSC_SUBMIT_MONITOR
	msc_submit_stats.read_ops++;
#endif

	while (retries-- > 0)
	{
		if (!msc_wait_ready(USB_MSC_WAIT_READY_TIMEOUT_MS))
		{
#if USB_MSC_SUBMIT_MONITOR
			msc_submit_stats.wait_ready_fail++;
#endif
			continue;
		}

		msc_chain_state.active = true;
		msc_chain_state.complete = false;
		msc_chain_state.submit_pending = true;
		msc_chain_state.result = RES_ERROR;
		msc_chain_state.op = MSC_CHAIN_READ;
		msc_chain_state.read_buff = buff;
		msc_chain_state.write_buff = NULL;
		msc_chain_state.sector = sector;
		msc_chain_state.remaining = count;
		msc_chain_state.submits_issued = 0;

		res = msc_wait_chain_complete(msc_chain_timeout_ms(count, 10000));
#if USB_MSC_LATENCY_MONITOR
		msc_latency_report(false);
#endif
#if USB_MSC_READ_PATTERN_MONITOR
		msc_read_pattern_report(false);
#endif
#if USB_MSC_SUBMIT_MONITOR
		msc_submit_report(false);
#endif
		if (res == RES_OK)
			return RES_OK;
	}

#if USB_MSC_LATENCY_MONITOR
	msc_latency_report(true);
#endif
#if USB_MSC_SUBMIT_MONITOR
	msc_submit_report(true);
#endif

	return res;
}

static DRESULT usb_disk_read_raw(BYTE *buff, LBA_t sector, UINT count)
{
#if USB_MSC_READ_PATTERN_MONITOR
	msc_read_pattern_record(sector, count);
#endif

#if USB_MSC_SEQ2_PREFETCH_ENABLE
	if (count == 1)
	{
		if (msc_seq2_prefetch.valid && sector == msc_seq2_prefetch.sector)
		{
			memcpy(buff, msc_seq2_prefetch.prefetched_sector, 512u);
			msc_seq2_prefetch.valid = false;
			if (msc_seq2_prefetch.have_last_single && sector == (msc_seq2_prefetch.last_single_sector + 1))
			{
				if (msc_seq2_prefetch.sequential_single_streak < 255)
					msc_seq2_prefetch.sequential_single_streak++;
			}
			else
			{
				msc_seq2_prefetch.sequential_single_streak = 0;
			}
			msc_seq2_prefetch.last_single_sector = sector;
			msc_seq2_prefetch.have_last_single = true;
			return RES_OK;
		}

		bool sequential_single = msc_seq2_prefetch.have_last_single && (sector == (msc_seq2_prefetch.last_single_sector + 1));
		bool use_pair_read = sequential_single && (msc_seq2_prefetch.sequential_single_streak >= USB_MSC_SEQ2_PREFETCH_MIN_STREAK);

		if (use_pair_read)
		{
			DRESULT pair_res = usb_disk_read_chain(msc_seq2_prefetch.pair_buffer, sector, 2);
			if (pair_res == RES_OK)
			{
				memcpy(buff, msc_seq2_prefetch.pair_buffer, 512u);
				memcpy(msc_seq2_prefetch.prefetched_sector, msc_seq2_prefetch.pair_buffer + 512u, 512u);
				msc_seq2_prefetch.sector = sector + 1;
				msc_seq2_prefetch.valid = true;
				if (msc_seq2_prefetch.sequential_single_streak < 255)
					msc_seq2_prefetch.sequential_single_streak++;
				msc_seq2_prefetch.last_single_sector = sector;
				msc_seq2_prefetch.have_last_single = true;
				return RES_OK;
			}
		}

		DRESULT single_res = usb_disk_read_chain(buff, sector, 1);
		if (single_res == RES_OK)
		{
			if (sequential_single)
			{
				if (msc_seq2_prefetch.sequential_single_streak < 255)
					msc_seq2_prefetch.sequential_single_streak++;
			}
			else
			{
				msc_seq2_prefetch.sequential_single_streak = 0;
			}
			msc_seq2_prefetch.last_single_sector = sector;
			msc_seq2_prefetch.have_last_single = true;
		}
		else
		{
			msc_seq2_prefetch_invalidate();
		}
		return single_res;
	}

	msc_seq2_prefetch_invalidate();
#endif

#if USB_MSC_READAHEAD_ENABLE
	if (count == 1)
	{
		if (msc_readahead_cache.valid &&
			sector >= msc_readahead_cache.start_sector &&
			sector < (msc_readahead_cache.start_sector + msc_readahead_cache.sector_count))
		{
			UINT idx = (UINT)(sector - msc_readahead_cache.start_sector);
			memcpy(buff, msc_readahead_cache.data + ((uint32_t)idx * 512u), 512u);
#if USB_MSC_READ_PATTERN_MONITOR
			msc_read_pattern_report(false);
#endif
			return RES_OK;
		}

		DRESULT res = usb_disk_read_chain(msc_readahead_cache.data, sector, USB_MSC_READAHEAD_SECTORS);
		if (res == RES_OK)
		{
			msc_readahead_cache.valid = true;
			msc_readahead_cache.start_sector = sector;
			msc_readahead_cache.sector_count = USB_MSC_READAHEAD_SECTORS;
			memcpy(buff, msc_readahead_cache.data, 512u);
#if USB_MSC_READ_PATTERN_MONITOR
			msc_read_pattern_report(false);
#endif
			return RES_OK;
		}

		msc_readahead_invalidate();
		res = usb_disk_read_chain(buff, sector, 1);
#if USB_MSC_READ_PATTERN_MONITOR
		msc_read_pattern_report(res != RES_OK);
#endif
		return res;
	}

	msc_readahead_invalidate();
#endif

	DRESULT res = usb_disk_read_chain(buff, sector, count);
#if USB_MSC_READ_PATTERN_MONITOR
	msc_read_pattern_report(res != RES_OK);
#endif
	return res;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT usb_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
	if (!count)
		return RES_PARERR;
	if (USBDriveStat & STA_NOINIT)
		return RES_NOTRDY;
	return usb_disk_read_raw(buff, sector, count);
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT usb_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
	if (!count)
		return RES_PARERR;
	if (USBDriveStat & STA_NOINIT)
		return RES_NOTRDY;

#if USB_MSC_SEQ2_PREFETCH_ENABLE
	msc_seq2_prefetch_invalidate();
#endif

#if USB_MSC_READAHEAD_ENABLE
	msc_readahead_invalidate();
#endif

	DRESULT res = RES_ERROR;
	int retries = USB_MSC_CHAIN_RETRIES;

#if USB_MSC_SUBMIT_MONITOR
	msc_submit_stats.write_ops++;
#endif

	while (retries-- > 0)
	{
		if (!msc_wait_ready(USB_MSC_WAIT_READY_TIMEOUT_MS))
		{
#if USB_MSC_SUBMIT_MONITOR
			msc_submit_stats.wait_ready_fail++;
#endif
			continue;
		}

		msc_chain_state.active = true;
		msc_chain_state.complete = false;
		msc_chain_state.submit_pending = true;
		msc_chain_state.result = RES_ERROR;
		msc_chain_state.op = MSC_CHAIN_WRITE;
		msc_chain_state.read_buff = NULL;
		msc_chain_state.write_buff = buff;
		msc_chain_state.sector = sector;
		msc_chain_state.remaining = count;
		msc_chain_state.submits_issued = 0;

		res = msc_wait_chain_complete(msc_chain_timeout_ms(count, 10000));
#if USB_MSC_SUBMIT_MONITOR
		msc_submit_report(false);
#endif
		if (res == RES_OK)
			return RES_OK;
	}

#if USB_MSC_SUBMIT_MONITOR
	msc_submit_report(true);
#endif

	return res;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT usb_disk_ioctl(
	BYTE cmd,  /* Control code */
	void *buff /* Buffer to send/receive data block */
)
{
	if (USBDriveStat & STA_NOINIT)
		return RES_NOTRDY;

	switch (cmd)
	{
	case CTRL_SYNC:
		/* USB transfers are complete when the callback fires */
		return RES_OK;

	case GET_SECTOR_COUNT:
		if (msc_dev_addr && tuh_msc_mounted(msc_dev_addr))
		{
			*(DWORD *)buff = tuh_msc_get_block_count(msc_dev_addr, 0);
			return RES_OK;
		}
		return RES_ERROR;

	case GET_BLOCK_SIZE:
		/* Erase block size in units of sectors - 1 is a safe default */
		*(DWORD *)buff = 1;
		return RES_OK;

	case GET_SECTOR_SIZE:
		if (msc_dev_addr && tuh_msc_mounted(msc_dev_addr))
		{
			*(WORD *)buff = (WORD)tuh_msc_get_block_size(msc_dev_addr, 0);
			return RES_OK;
		}
		return RES_ERROR;

	case CTRL_POWER:
		USBDriveStat |= STA_NOINIT;
		return RES_OK;

	default:
		return RES_PARERR;
	}
}

