/* USBTouch.c - the PicoMite's USB touch screen (input/USBKeyboard.c, graphics/Pointer.c's gestures), copied by
   tools/port_usbtouch.py: do not edit, run it again. What the CMM2 changes is listed there. */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "tusb.h"
#include "keyboard.h"
#include "USBGamepad.h"
#include <math.h>

#define time_us_64() ((uint64_t)Getuptime())
#define GUICONTROLS
extern void USBMessage(const char *m);

/* USB multi-touch -> generic "touch panel" integration.
   usb_touch_present is true between mount and umount of a touch device;
   usb_touch_active goes true on every report whose contact 0 has tip==1.
   usb_touch_x / usb_touch_y always hold contact 0's screen-scaled
   coordinates. The timer-callback edge detector (PicoMite.c) reads these
   alongside TOUCH_DOWN so the GUI control machinery doesn't care whether
   the input came from a resistive/capacitive panel or a USB touchscreen.
   GetTouch() in GUI.c returns them when the resistive panel is absent.

   usb_touch_last_us tracks the wall-clock of the most recent report
   that set usb_touch_active = true. The 1 ms timer callback in
   PicoMite.c uses it as a no-report watchdog: some touch controllers
   (notably the one used in the Pimoroni Pico Plus 2W test rig) fail
   to send a release report (count=0 / tip=0) under specific timing
   patterns, leaving usb_touch_active stuck true. Without the
   watchdog ProcessTouch's static `repeat` for spinners then never
   clears and the GUI machinery thinks the touch is held forever. */
volatile bool usb_touch_present = false;
volatile bool usb_touch_active = false;
volatile int16_t usb_touch_x = 0;
volatile int16_t usb_touch_y = 0;
volatile uint64_t usb_touch_last_us = 0;

/* Contact 1 — exposed via TOUCH(X2) / TOUCH(Y2), mirroring the
   Option.TOUCH_CAP capacitive multi-touch API. usb_touch_active2 is
   true only when contact 1 is present AND has tip=1. */
volatile bool usb_touch_active2 = false;
volatile int16_t usb_touch_x2 = 0;
volatile int16_t usb_touch_y2 = 0;

/* All live contacts (tip=1), compacted to the front, exposed via
   TOUCH(XN n) / TOUCH(YN n) for n = 1..usb_touch_count. Contacts 0 and 1
   duplicate usb_touch_x/y and usb_touch_x2/y2 so the legacy accessors are
   unaffected. Refreshed wholesale on every decoded report by touch_publish. */
volatile uint8_t usb_touch_count = 0;
volatile int16_t usb_touch_xn[MAX_TOUCH_CONTACTS] = {0};
volatile int16_t usb_touch_yn[MAX_TOUCH_CONTACTS] = {0};

/* ============================================================================
 * USB multi-touch HID — descriptor parser + report analyser
 * ============================================================================
 * Targets the Windows-Precision-Touchscreen-style descriptor layout that
 * essentially every multi-touch USB controller ships with: a Touch
 * Screen application collection (Digitizer page 0x0D / usage 0x04)
 * containing N Finger logical collections (usage 0x22), each declaring
 * Tip Switch (0x42), In Range (0x32), Contact Identifier (0x51), X
 * (Generic Desktop 0x30), Y (0x31). A top-level Contact Count (0x54)
 * carries the per-report live-contact count.
 *
 * The parser walks the descriptor once at enumeration to capture
 * field offsets; the analyser then extracts contact data from each
 * input report by bit-offset rather than re-parsing.
 */

/* Bit-aligned read from a byte buffer. bit_count up to 32. */
static uint32_t touch_read_bits(const uint8_t *bytes, uint16_t bit_offset, uint8_t bit_count)
{
	uint32_t result = 0;
	for (uint8_t i = 0; i < bit_count; i++)
	{
		uint16_t b = bit_offset + i;
		if (bytes[b >> 3] & (1u << (b & 7)))
		{
			result |= (1u << i);
		}
	}
	return result;
}

/**
 * Walk a HID report descriptor looking for the Windows Touch layout.
 * Returns true and fills *info if the descriptor describes a touch
 * screen with at least one Finger collection; otherwise returns false
 * and leaves *info zeroed.
 */
bool analyze_touch_descriptor(const uint8_t *desc_report, uint16_t desc_len, touch_info_t *info)
{
	if (!desc_report || !info || desc_len == 0)
		return false;

	memset(info, 0, sizeof(*info));

	/* Parser state. */
	uint8_t usage_page = 0;
	uint16_t last_usage = 0;   /* most recent Local Usage item */
	uint8_t report_size = 0;   /* Global Report Size */
	uint8_t report_count = 0;  /* Global Report Count */
	int32_t logical_max = 0;   /* Global Logical Maximum (last value seen) */
	uint16_t bit_position = 0; /* bit offset within current Report ID's report */
	int collection_depth = 0;

	/* Report-ID tracking. A typical Precision Touchscreen descriptor
	   declares multiple Report IDs (ID 1 for touch Input, ID 4 for
	   Feature mode-select, etc.). Each Report ID has its own report
	   transmitted independently with the ID byte prepended. We need
	   to track which ID contains the Finger collections and only
	   accumulate bit positions / capture field offsets while parsing
	   THAT report's items. */
	int current_report_id = 0; /* 0 = no Report ID encountered yet */
	int finger_report_id = -1; /* ID we want to capture, locked at first Finger */

	/* Finger-collection tracking. */
	bool in_finger = false;
	int finger_start_depth = -1;
	uint16_t finger_start_bit = 0;
	uint8_t finger_count = 0;
	bool saw_touchscreen_root = false;

	/* Pointer (Generic Desktop Mouse/Pointer) fallback-collection tracking.
	   ptr_uq is a small usage queue so an X/Y pair declared as one Input
	   with a usage array (09 30 09 31 ... Report Count 2) is distributed
	   across the two 16-bit fields — the common mouse idiom. Reset on every
	   Main item; only populated while inside the pointer collection. */
	bool in_pointer = false;
	int pointer_start_depth = -1;
	int pointer_report_id = -1;
	bool got_pointer_button = false;
	uint16_t ptr_uq[8];
	uint8_t ptr_uq_n = 0;

	/* Device Configuration collection (usage 0x0E) — hosts the Input Mode
	   (Device Mode 0x52) Feature used to switch the panel to multi-touch. */
	bool in_device_config = false;
	int device_config_depth = -1;

	for (uint16_t i = 0; i < desc_len;)
	{
		uint8_t prefix = desc_report[i++];
		uint8_t bSize = prefix & 0x03;
		uint8_t bType = (prefix >> 2) & 0x03;
		uint8_t bTag = (prefix >> 4) & 0x0F;

		uint32_t data = 0;
		for (uint8_t j = 0; j < bSize && (i + j) < desc_len; j++)
			data |= ((uint32_t)desc_report[i + j]) << (j * 8);
		i += bSize;

		if (bType == 1) /* Global */
		{
			switch (bTag)
			{
			case 0:
				usage_page = data & 0xFF;
				break;
			case 2:
				logical_max = (int32_t)data;
				break; /* Logical Maximum */
			case 7:
				report_size = data & 0xFF;
				break; /* Report Size */
			case 8:	   /* Report ID */
				info->uses_report_id = true;
				current_report_id = data & 0xFF;
				/* New Report ID starts a fresh report — reset the
				   bit-position counter. Each Report ID's report is
				   transmitted independently. */
				bit_position = 0;
				break;
			case 9:
				report_count = data & 0xFF;
				break; /* Report Count */
			}
		}
		else if (bType == 2) /* Local */
		{
			if (bTag == 0)
			{
				last_usage = data & 0xFFFF; /* Usage */
				if (in_pointer && ptr_uq_n < (uint8_t)(sizeof(ptr_uq) / sizeof(ptr_uq[0])))
					ptr_uq[ptr_uq_n++] = last_usage;
			}
		}
		else if (bType == 0) /* Main */
		{
			if (bTag == 10) /* Collection */
			{
				collection_depth++;
				/* The application collection tells us this is a touch
				   screen. Required for valid=true at the end. */
				if (collection_depth == 1 && usage_page == 0x0D && last_usage == 0x04)
					saw_touchscreen_root = true;
				/* Generic Desktop Mouse(0x02)/Pointer(0x01) application
				   collection — the single-finger pointer fallback some
				   digitizers stream instead of the multitouch report.
				   Capture its layout in parallel; the report ID is declared
				   just inside it. */
				if (collection_depth == 1 && usage_page == 0x01 &&
					(last_usage == 0x01 || last_usage == 0x02) && !in_pointer)
				{
					in_pointer = true;
					pointer_start_depth = collection_depth;
				}
				/* Device Configuration collection — carries the Input Mode
				   (Device Mode) Feature whose Report ID we need to write. */
				if (usage_page == 0x0D && last_usage == 0x0E && !in_device_config)
				{
					in_device_config = true;
					device_config_depth = collection_depth;
				}
				/* Finger collection — capture start bit, then descend
				   so per-contact fields populate relative offsets.
				   Lock finger_report_id to the Report ID that hosts
				   the first Finger we see; ignore Fingers in other
				   Report IDs (e.g. Feature-report duplicates). */
				if (usage_page == 0x0D && last_usage == 0x22 && !in_finger)
				{
					if (finger_report_id == -1)
					{
						finger_report_id = current_report_id;
						info->report_id = (uint8_t)current_report_id;
					}
					if (current_report_id == finger_report_id)
					{
						in_finger = true;
						finger_start_depth = collection_depth;
						finger_start_bit = bit_position;
					}
				}
			}
			else if (bTag == 12) /* End Collection */
			{
				if (in_finger && collection_depth == finger_start_depth)
				{
					if (finger_count == 0)
					{
						info->contact_stride_bits =
							bit_position - finger_start_bit;
						info->first_contact_bit_offset = finger_start_bit;
					}
					finger_count++;
					in_finger = false;
				}
				if (in_pointer && collection_depth == pointer_start_depth)
					in_pointer = false;
				if (in_device_config && collection_depth == device_config_depth)
					in_device_config = false;
				collection_depth--;
			}
			else if (bTag == 8) /* Input */
			{
				uint16_t field_bits = (uint16_t)report_size * report_count;

				/* Only capture field offsets when we're in the Report
				   ID that owns the Finger collections. Before any
				   Finger has been seen we capture everything (the
				   first Finger we encounter will lock the ID). */
				bool capture = (finger_report_id == -1) || (current_report_id == finger_report_id);

				if (capture && in_finger && finger_count == 0)
				{
					/* Capture field offsets relative to start of contact
					   block. Only record the first finger's layout — the
					   stride takes care of the rest. */
					uint16_t rel = bit_position - finger_start_bit;
					if (usage_page == 0x0D)
					{
						if (last_usage == 0x42)
							info->tip_switch_bit_offset = rel;
						else if (last_usage == 0x32)
							info->in_range_bit_offset = rel;
						else if (last_usage == 0x51)
						{
							info->contact_id_bit_offset = rel;
							info->contact_id_bits = report_size;
						}
					}
					else if (usage_page == 0x01) /* Generic Desktop */
					{
						if (last_usage == 0x30)
						{
							info->x_bit_offset = rel;
							info->x_bits = report_size;
							info->x_logical_max = logical_max;
						}
						else if (last_usage == 0x31)
						{
							info->y_bit_offset = rel;
							info->y_bits = report_size;
							info->y_logical_max = logical_max;
						}
					}
				}
				else if (capture && !in_finger)
				{
					/* Top-level (within touch report): Contact Count
					   is the only field we need; Scan Time, button
					   indicators etc. are ignored. */
					if (usage_page == 0x0D && last_usage == 0x54)
					{
						info->contact_count_bit_offset = bit_position;
						info->contact_count_bits = report_size;
					}
				}

				/* Pointer fallback capture (runs independently of the
				   finger 'capture' flag — the Mouse collection has its own
				   report ID, so bit_position has already been reset for it).
				   Button 1 is the LSB of the first Button field; X/Y are the
				   ABSOLUTE Generic Desktop axes (skip relative deltas — a
				   relative mouse has no screen mapping). X and Y may share
				   one Input via a usage array, so walk the per-field usages
				   from the queue and place each by its own bit offset. */
				if (in_pointer)
				{
					if (pointer_report_id == -1)
						pointer_report_id = current_report_id;
					if (usage_page == 0x09 && !got_pointer_button)
					{
						info->pointer_button_bit_offset = bit_position;
						got_pointer_button = true;
					}
					else if (usage_page == 0x01 && !(data & 0x04)) /* Absolute */
					{
						for (uint8_t f = 0; f < report_count; f++)
						{
							uint16_t u = (f < ptr_uq_n) ? ptr_uq[f]
														: (ptr_uq_n ? ptr_uq[ptr_uq_n - 1] : last_usage);
							uint16_t off = bit_position + (uint16_t)f * report_size;
							if (u == 0x30)
							{
								info->pointer_x_bit_offset = off;
								info->pointer_x_bits = report_size;
								info->pointer_x_logical_max = logical_max;
							}
							else if (u == 0x31)
							{
								info->pointer_y_bit_offset = off;
								info->pointer_y_bits = report_size;
								info->pointer_y_logical_max = logical_max;
							}
						}
					}
				}

				bit_position += field_bits;
				/* Local usage items are 'consumed' by the next Main item. */
				last_usage = 0;
			}
			else if (bTag == 9) /* Output / Feature */
			{
				/* Still need to consume bit_position for Feature/Output
				   reports that share the same Report ID — but only
				   Input bits matter for the live report layout. */
				last_usage = 0;
			}
			else if (bTag == 11) /* Feature */
			{
				/* The Feature inside the Device Configuration collection is
				   the Input Mode (Device Mode) selector. Capture its Report
				   ID so we can write it to enable multi-touch. */
				if (in_device_config && !info->has_input_mode)
				{
					info->has_input_mode = true;
					info->input_mode_report_id = (uint8_t)current_report_id;
				}
				/* Windows bring-up Feature reports, captured by usage so the
				   GET_FEATURE handshake isn't tied to one panel's report IDs:
				   Contact Count Maximum (0x55, digitizer page) and the
				   Microsoft certification blob (0xC5, distinctive enough to
				   match on usage alone since the vendor page is truncated). */
				if (usage_page == 0x0D && last_usage == 0x55 && !info->contact_count_max_report_id)
					info->contact_count_max_report_id = (uint8_t)current_report_id;
				if (last_usage == 0xC5 && !info->cert_blob_report_id)
					info->cert_blob_report_id = (uint8_t)current_report_id;
				last_usage = 0;
			}
			/* Every Main item consumes the pending Local usages. */
			ptr_uq_n = 0;
		}
	}

	/* Total Input report length for the Finger-bearing Report ID,
	   rounded up to a byte, plus the report-ID byte if present.
	   bit_position holds whatever the last Report ID's accumulated
	   length was — if the descriptor ended on a non-finger report, we
	   may have a stale value. Recompute from first_contact_bit_offset
	   + max_contacts * stride + post-contacts fields (just the
	   contact-count bit) as a safer estimate. */
	if (finger_count > 0)
	{
		uint16_t after_contacts = info->first_contact_bit_offset + finger_count * info->contact_stride_bits;
		uint16_t after_cc = info->contact_count_bit_offset + info->contact_count_bits;
		uint16_t end_bits = (after_contacts > after_cc) ? after_contacts : after_cc;
		info->report_length_bytes = (end_bits + 7) / 8;
		if (info->uses_report_id)
			info->report_length_bytes += 1;
	}
	info->max_contacts = finger_count;

	/* Finalise the pointer fallback. Require its own report ID (distinct
	   from the multitouch one) plus absolute X/Y — a relative mouse has no
	   logical max and can't be scaled to the screen, so skip those. */
	if (pointer_report_id >= 0 && pointer_report_id != finger_report_id &&
		got_pointer_button && info->pointer_x_bits > 0 && info->pointer_y_bits > 0 &&
		info->pointer_x_logical_max > 0 && info->pointer_y_logical_max > 0)
	{
		info->has_pointer_fallback = true;
		info->pointer_report_id = (uint8_t)pointer_report_id;
		uint16_t bx = info->pointer_button_bit_offset + 1;
		uint16_t ex = info->pointer_x_bit_offset + info->pointer_x_bits;
		uint16_t ey = info->pointer_y_bit_offset + info->pointer_y_bits;
		uint16_t end_bits = bx;
		if (ex > end_bits)
			end_bits = ex;
		if (ey > end_bits)
			end_bits = ey;
		info->pointer_report_length_bytes = (end_bits + 7) / 8;
		if (info->uses_report_id)
			info->pointer_report_length_bytes += 1;
	}

	info->valid = (saw_touchscreen_root && finger_count > 0 && info->x_bits > 0 && info->y_bits > 0);
	return info->valid;
}

/* Scale a raw device-logical axis value to current display coords, with
   the same clip-to-range / fall-back-when-uninitialised rules used for the
   multitouch contacts. res is HRes or VRes. */
static int16_t touch_scale_axis(uint32_t raw, int32_t logical_max, int res)
{
	if (logical_max > 0 && res > 0)
	{
		if (raw > (uint32_t)logical_max)
			raw = (uint32_t)logical_max;
		int32_t v = (int32_t)((raw * (uint32_t)res) / (uint32_t)logical_max);
		if (v >= res)
			v = res - 1;
		return (int16_t)v;
	}
	return (int16_t)raw;
}

/* Publish a decoded report to the generic touch-panel state watched by the
   PicoMite.c edge detector (TouchDown/TouchUp) and GetTouch() in GUI.c, and
   run the pinch / long-press gesture machine. Shared by the multitouch and
   pointer-fallback decode paths. Uses out->count + out->contacts[]. */
static void touch_publish(touch_report_t *out)
{
	/* A contact reported with tip=0 (hover) does NOT count as "pressed".
	   Stamp the timestamp on EVERY report (including count=0 releases) so
	   the watchdog only fires when reports actually stop arriving. */
	usb_touch_last_us = time_us_64();
	/* (the CMM2, until PicoMite #144: the contacts in a stable order, followed by their IDs. A panel that
	   puts a newly landed finger in its first slot (27C0:0859) made contact 0 the new finger; one that keeps
	   the order, or reports no IDs, is unchanged) */
	{
		static uint8_t prev_id[2] = {0xFF, 0xFF};
		for (int p = 0; p < 2; p++)
			for (int k = p + 1; k < out->count; k++)
				if (out->contacts[k].id == prev_id[p] && out->contacts[p].id != prev_id[p])
				{
					touch_contact_t t = out->contacts[p];
					out->contacts[p] = out->contacts[k];
					out->contacts[k] = t;
				}
		prev_id[0] = out->count > 0 ? out->contacts[0].id : 0xFF;
		prev_id[1] = out->count > 1 ? out->contacts[1].id : 0xFF;
	}
	if (out->count > 0 && out->contacts[0].tip)
	{
		usb_touch_x = out->contacts[0].x;
		usb_touch_y = out->contacts[0].y;
		usb_touch_active = true;
	}
	else
	{
		usb_touch_active = false;
	}
	/* Contact 1 mirrors the Option.TOUCH_CAP X2/Y2 API. */
	if (out->count >= 2 && out->contacts[1].tip)
	{
		usb_touch_x2 = out->contacts[1].x;
		usb_touch_y2 = out->contacts[1].y;
		usb_touch_active2 = true;
	}
	else
	{
		usb_touch_active2 = false;
	}

	/* Publish every live contact for TOUCH(XN n)/TOUCH(YN n). out->count is
	   the tip=1 contacts compacted to the front, so xn[0..count-1] are valid.
	   Set the count last so a BASIC reader never sees coords from a contact
	   that the count says is absent. */
	{
		uint8_t nc = out->count;
		if (nc > MAX_TOUCH_CONTACTS)
			nc = MAX_TOUCH_CONTACTS;
		for (uint8_t k = 0; k < nc; k++)
		{
			usb_touch_xn[k] = out->contacts[k].x;
			usb_touch_yn[k] = out->contacts[k].y;
		}
		usb_touch_count = nc;
	}

#ifdef GUICONTROLS
	/* Two-finger pinch + long-press gesture analysis. The gesture state
	   machine lives in Draw.c under #ifdef GUICONTROLS, so builds without
	   GUI controls (e.g. VGAUSB) don't link these symbols — skip the
	   calls. Raw usb_touch_x/y is still available to BASIC. Pinch is "live"
	   while BOTH contacts have tip=1; usb_touch_x/y still hold the last
	   position even after active goes false, so the end-position read is
	   valid right at the transition. */
	{
		static bool pinch_was_both = false;
		bool now_both = usb_touch_active && usb_touch_active2;
		if (now_both && !pinch_was_both)
		{
			touch_gesture_pinch_start(usb_touch_x, usb_touch_y,
									  usb_touch_x2, usb_touch_y2);
		}
		else if (!now_both && pinch_was_both)
		{
			touch_gesture_pinch_end(usb_touch_x, usb_touch_y,
									usb_touch_x2, usb_touch_y2);
		}
		pinch_was_both = now_both;
	}
	/* Long-press detection during the hold. Runs on every report so a
	   long-press fires the moment the held threshold is crossed, without
	   waiting for the lift. Skipped when contact 1 is also active. */
	touch_gesture_tick(usb_touch_x, usb_touch_y,
					   usb_touch_active && !usb_touch_active2);
#endif /* GUICONTROLS */
}

/* Decode a single-contact pointer-fallback report (dual-mode digitizer in
   mouse mode). `bytes` is the payload with the report-ID byte already
   stripped. Button 1 = tip; absolute X/Y scale like a contact. */
static bool process_pointer_report(const uint8_t *bytes, uint16_t payload_len,
								   const touch_info_t *info, touch_report_t *out)
{
	uint32_t need = (uint32_t)info->pointer_button_bit_offset + 1;
	uint32_t nx = (uint32_t)info->pointer_x_bit_offset + info->pointer_x_bits;
	uint32_t ny = (uint32_t)info->pointer_y_bit_offset + info->pointer_y_bits;
	if (nx > need)
		need = nx;
	if (ny > need)
		need = ny;
	if ((uint32_t)payload_len * 8u < need)
	{
		out->count = 0;
		return false;
	}

	bool tip = touch_read_bits(bytes, info->pointer_button_bit_offset, 1) != 0;
	touch_contact_t *tc = &out->contacts[0];
	tc->tip = tip;
	tc->in_range = tip;
	tc->id = 0;
	uint32_t raw_x = touch_read_bits(bytes, info->pointer_x_bit_offset, info->pointer_x_bits);
	uint32_t raw_y = touch_read_bits(bytes, info->pointer_y_bit_offset, info->pointer_y_bits);
	tc->x = touch_scale_axis(raw_x, info->pointer_x_logical_max, HRes);
	tc->y = touch_scale_axis(raw_y, info->pointer_y_logical_max, VRes);
	out->count = tip ? 1 : 0;

	touch_publish(out);
	return true;
}

/**
 * Decode an Input report into a touch_report_t, using offsets captured
 * by analyze_touch_descriptor(). `report` is the raw bytes as TinyUSB
 * delivers them (which include the report-ID byte iff the device
 * uses report IDs). Returns true if the report was decoded.
 */
bool process_touch_report(const uint8_t *report, uint16_t len,
						  const touch_info_t *info, touch_report_t *out)
{
	if (!report || !info || !out || !info->valid)
		return false;

	const uint8_t *bytes = report;
	uint16_t payload_len = len;

	/* If the device uses report IDs, the first byte is the report ID.
	   A dual-mode digitizer streams its Mouse-collection report ID for a
	   single finger — route that to the pointer-fallback decoder. Reports
	   for any other report ID (config / feature mirrors) are ignored. */
	if (info->uses_report_id)
	{
		if (payload_len < 1)
		{
			out->count = 0;
			return false;
		}
		if (info->has_pointer_fallback && bytes[0] == info->pointer_report_id)
			return process_pointer_report(bytes + 1, payload_len - 1, info, out);
		if (bytes[0] != info->report_id)
		{
			out->count = 0;
			return false;
		}
		bytes++;
		payload_len--;
	}

	/* Reject a report too short to contain the contact-count field. A
	   device whose report exceeds CFG_TUH_HID_EPIN_BUFSIZE arrives
	   fragmented across callbacks; a leading fragment that omits the
	   contact-count byte would otherwise decode as count=0 and silently
	   swallow the touch. Better to drop the fragment than misread it. */
	if ((uint32_t)payload_len * 8u <
		(uint32_t)info->contact_count_bit_offset + info->contact_count_bits)
	{
		out->count = 0;
		return false;
	}

	/* The Contact Count field tells us how many contact slots the report
	   carries — but on many panels it counts contacts still being TRACKED,
	   including fingers that have lifted (reported with tip=0) and aren't
	   released yet. So it doesn't fall as fingers lift. We therefore use it
	   only to bound the slot scan (so a fixed footer past the live slots
	   isn't misread as a contact) and report out->count as the number of
	   slots actually TOUCHING (tip=1), compacted to the front so contact 0/1
	   are the live fingers regardless of which hardware slot they occupy. */
	uint8_t scan = touch_read_bits(bytes,
								   info->contact_count_bit_offset,
								   info->contact_count_bits);
	if (scan > info->max_contacts)
		scan = info->max_contacts;
	if (scan > MAX_TOUCH_CONTACTS)
		scan = MAX_TOUCH_CONTACTS;

	uint8_t live = 0;
	for (uint8_t c = 0; c < scan; c++)
	{
		uint16_t base = info->first_contact_bit_offset + (uint16_t)c * info->contact_stride_bits;
		if (touch_read_bits(bytes, base + info->tip_switch_bit_offset, 1) == 0)
			continue; /* lifted / hovering contact — not a live touch */
		touch_contact_t *tc = &out->contacts[live++];
		tc->tip = true;
		tc->in_range = touch_read_bits(bytes, base + info->in_range_bit_offset, 1) != 0;
		tc->id = (uint8_t)touch_read_bits(bytes, base + info->contact_id_bit_offset,
										  info->contact_id_bits);
		uint32_t raw_x = touch_read_bits(bytes, base + info->x_bit_offset, info->x_bits);
		uint32_t raw_y = touch_read_bits(bytes, base + info->y_bit_offset, info->y_bits);
		/* Scale device-logical coords (0..x_logical_max / 0..y_logical_max)
		   to the current display-mode coords (HRes / VRes). Devices
		   typically report values 0..(logical_max-1) inclusive, so
		   dividing by logical_max produces a value 0..HRes-1 / 0..VRes-1
		   that aligns with screen-pixel addressing used by everything
		   else (DrawPixel, BLIT, GUI hit-tests). On mode change (e.g.
		   SCREEN 2 on HDMIBTH drops HRes from 1024 to 256) the next
		   report decodes against the new dimensions automatically.
		   Clip raw to logical_max so a device briefly reporting beyond
		   its declared range can't produce scaled > HRes-1. If the
		   display isn't initialised yet (HRes/VRes==0), fall back to
		   the raw value. */
		if (info->x_logical_max > 0 && HRes > 0)
		{
			if (raw_x > (uint32_t)info->x_logical_max)
				raw_x = info->x_logical_max;
			tc->x = (int16_t)((raw_x * (uint32_t)HRes) / (uint32_t)info->x_logical_max);
			if (tc->x >= HRes)
				tc->x = HRes - 1;
		}
		else
		{
			tc->x = (int16_t)raw_x;
		}
		if (info->y_logical_max > 0 && VRes > 0)
		{
			if (raw_y > (uint32_t)info->y_logical_max)
				raw_y = info->y_logical_max;
			tc->y = (int16_t)((raw_y * (uint32_t)VRes) / (uint32_t)info->y_logical_max);
			if (tc->y >= VRes)
				tc->y = VRes - 1;
		}
		else
		{
			tc->y = (int16_t)raw_y;
		}
	}
	out->count = live; /* number of contacts actually touching (tip=1) */

	touch_publish(out);
	return true;
}

#define dump_touch_descriptor(desc, len, info) ((void)0)
#define reset_touch_raw_dump() ((void)0)
#define dump_touch_report(info, report, len, out) ((void)0)

/**
 * Expected total length (bytes, including the report-ID byte) of the Input
 * report identified by the leading byte `rid`. Used by the fragment
 * reassembler to know when a multi-packet report is complete. Returns 0
 * for an unrecognised report ID — the caller then treats the chunk as a
 * self-contained report (process it as-is).
 */
static uint16_t touch_expected_len(const touch_info_t *info, uint8_t rid)
{
	if (!info->uses_report_id)
		return info->report_length_bytes;
	if (rid == info->report_id)
		return info->report_length_bytes;
	if (info->has_pointer_fallback && rid == info->pointer_report_id)
		return info->pointer_report_length_bytes;
	return 0;
}

/**
 * Frame touch Input reports out of the raw interrupt-IN byte stream and
 * decode each complete one. Needed because the RP2350 USB host doesn't hand
 * us one tidy report per callback:
 *
 *   - Long reports are SPLIT: it completes the transfer one USB packet
 *     (≤ wMaxPacketSize, 64 bytes) at a time regardless of
 *     CFG_TUH_HID_EPIN_BUFSIZE, so a 10-contact 84-byte report arrives as
 *     64 + 20. A leading fragment lacks the contact-count byte → decoded 0.
 *   - Fast panels COALESCE: several short reports land in one callback
 *     (e.g. the ILI2132 delivering two 6-byte pointer reports as one 12-byte
 *     chunk), sometimes with a couple of stray bytes between them so the
 *     reports don't sit on clean boundaries. Decoding only the first lost
 *     samples and left usb_touch_active stale (a rejected fragment returns
 *     before touch_publish() runs).
 *
 * Both are handled by accumulating bytes in a stream buffer and repeatedly
 * pulling reports off the front: the leading byte is a report ID whose total
 * length touch_expected_len() knows. With enough bytes we decode and advance;
 * a known ID with too few bytes waits for the next callback (the split case);
 * an unrecognised leading byte means we're mis-aligned, so skip one byte to
 * resync (the stray-byte case). A single static buffer serves the normally
 * single active touch panel; a different device resets it. Always-on.
 */
static void touch_reassemble(uint8_t dev_addr, uint8_t instance,
							 const uint8_t *report, uint16_t len,
							 const touch_info_t *info, touch_report_t *out)
{
	static uint8_t buf[256];
	static uint16_t fill = 0;
	static uint8_t owner_addr = 0xFF, owner_inst = 0xFF;

	/* A different device (or first ever call) restarts the stream. */
	if (owner_addr != dev_addr || owner_inst != instance)
	{
		owner_addr = dev_addr;
		owner_inst = instance;
		fill = 0;
	}

	/* Append. If the buffer would overflow we've stalled on undecodable
	   data — drop it and resync on the fresh chunk rather than wedge. */
	if ((uint32_t)fill + len > sizeof(buf))
		fill = 0;
	uint16_t copy = (len > sizeof(buf)) ? (uint16_t)sizeof(buf) : len;
	memcpy(buf + fill, report, copy);
	fill += copy;

	/* Pull complete reports off the front. */
	uint16_t pos = 0;
	while (pos < fill)
	{
		uint16_t exp = touch_expected_len(info, buf[pos]);
		if (exp == 0)
		{
			pos++; /* not a known report ID → mis-aligned, resync */
			continue;
		}
		if (fill - pos < exp)
			break; /* known report, not all here yet → await next callback */
		process_touch_report(buf + pos, exp, info, out);
		pos += exp;
	}

	/* Keep the unconsumed tail for next time. */
	if (pos > 0)
	{
		if (pos < fill)
			memmove(buf, buf + pos, fill - pos);
		fill -= pos;
	}

	/* A report that fits one USB packet (<= wMaxPacketSize, 64) always
	   arrives whole in a single callback — so any leftover after extracting
	   the whole reports is padding or stray bytes and MUST be dropped. If we
	   carried it, a payload byte that happens to equal a report ID (these
	   62-byte reports contain 0x04 in their footer) could anchor the parser
	   one report out of phase and, because that byte is a "valid" report ID,
	   it would never resync — every report then decodes as garbage/count=0.
	   Only a genuinely fragmented report (length > 64, still awaiting later
	   packets) is legitimately carried across callbacks. */
	if (fill > 0)
	{
		uint16_t exp0 = touch_expected_len(info, buf[0]);
		if (!(exp0 > 64 && fill < exp0))
			fill = 0;
	}
}

/* Scratch buffer for the touch digitizer-init GET_FEATURE reads. Sized for
   the largest one (the 256-byte certification blob, report ID 6). The data
   is only needed so the device sees the read happen — we don't parse it. */
static uint8_t touch_feature_buf[256];

/* Completion of a GET_FEATURE control transfer. Used to sequence the touch
   digitizer-init handshake (see hid_app_task): advance to the next read, or
   mark it done. Independent of report polling. */
void USBTouchGetReportComplete(uint8_t dev_addr, uint8_t instance,
									uint8_t report_id, uint8_t report_type, uint16_t len)
{
	for (int i = 0; i < 4; i++)
	{
		if (HID[i].Device_address == dev_addr && HID[i].Device_instance == instance &&
			HID[i].Device_type == USB_TOUCH)
		{
#ifdef TOUCH_DEBUG
			/* Dump the raw read value BEFORE we modify it. */
			if (!CurrentLinePtr)
			{
				MMPrintString("Touch GET_FEATURE id=");
				PInt(report_id);
				MMPrintString(" len=");
				PInt(len);
				if (len >= 2)
				{
					MMPrintString(" data=");
					PIntH(touch_feature_buf[0]);
					MMPrintString(",");
					PIntH(touch_feature_buf[1]);
				}
				PRet();
			}
#endif
			if (HID[i].touch_init_step == 2)
				HID[i].touch_init_step = 3; /* got blob → read contact-count-max */
			else if (HID[i].touch_init_step == 4)
				HID[i].touch_init_step = 0; /* handshake complete */
			else if (HID[i].touch_init_step == 12)
			{
				/* Read-modify-write Input Mode: we just read the current
				   [Device Mode, Device Identifier]; force Device Mode to 0x02
				   (touchscreen) but keep the identifier the panel returned,
				   then write it back (step 13). */
				touch_feature_buf[0] = 0x02;
				HID[i].touch_init_step = 13;
			}
			break;
		}
	}
}

/* Completion of a SET_FEATURE control transfer — used to clear the touch
   Input Mode write's in-flight state. */
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance,
									uint8_t report_id, uint8_t report_type, uint16_t len)
{
	for (int i = 0; i < 4; i++)
	{
		if (HID[i].Device_address == dev_addr && HID[i].Device_instance == instance &&
			HID[i].Device_type == USB_TOUCH && HID[i].touch_init_step == 11)
		{
			HID[i].touch_init_step = 0; /* Input Mode write done */
#ifdef TOUCH_DEBUG
			if (!CurrentLinePtr)
			{
				MMPrintString("Touch SET_FEATURE InputMode id=");
				PInt(report_id);
				MMPrintString(" len=");
				PInt(len);
				PRet();
			}
#endif
			break;
		}
	}
}

/* Periodic touch handshake re-assertion. 0 = disabled (A/B test whether the
   panels actually revert after the one-shot bring-up). 1 = re-assert every 2 s. */
#define TOUCH_KEEPALIVE 0

/* hid_app_task's part for a touch screen (the CMM2's hid_app_task calls this for each active slot) */
void USBTouchTask(int i)
{
	uint64_t timenow = time_us_64();
	(void)timenow;
		/* Keep-alive: some panels (ILI2132) were seen to stay in digitizer
		   mode only a few seconds after the bring-up handshake, then revert.
		   Re-asserting the handshake every 2 s prevents that. Currently
		   DISABLED for an A/B test — set TOUCH_KEEPALIVE to 1 (defined above
		   hid_app_task) to re-enable. If the panel turns out to revert, the
		   preferred fix is a reactive version (re-assert only on a stall). */
#if TOUCH_KEEPALIVE
		if (HID[i].Device_type == USB_TOUCH && HID[i].touch_init_step == 0 &&
			(HID[i].touch_info.has_input_mode || HID[i].touch_info.has_pointer_fallback) &&
			(timenow - HID[i].touch_init_us) > 2000000ULL)
		{
			HID[i].touch_init_us = timenow;
			HID[i].touch_init_step = HID[i].touch_info.has_input_mode ? 10 : 1;
		}
#endif
		/* Touch digitizer-init handshake. Two mechanisms, by step value:
			 steps 10/13 -> STANDARD: read-modify-write Input Mode (Device
			   Mode) to 0x02 to switch a Windows-compliant panel into
			   multi-touch. Read (10) the current [mode, identifier], force
			   mode=0x02 keeping the identifier, write back (13). The read
			   first matches Linux hid-multitouch's FORCE_GET_FEATURE: some
			   panels reject a blind write that zeroes the identifier.
			 steps 1/3 -> GET_FEATURE the Windows bring-up reports (cert
			   blob, then contact-count-max) at report IDs captured from the
			   descriptor, for dual-mode panels that lack Input Mode (ILI2132).
		   All use EP0 — if it can't be issued this cycle (control pipe busy)
		   we retry next time; in-flight steps (2/4/11/12) are cleared only by
		   the completion callbacks. A panel that STALLs the request simply
		   stops here without affecting report polling below. */
		if (HID[i].Device_type == USB_TOUCH && HID[i].touch_init_step == 10)
		{
			/* Read the current Input Mode feature ([Device Mode, Device
			   Identifier]) into the buffer; the GET completion forces
			   mode=0x02 and advances to the write (step 13). */
			if (tuh_hid_get_report(HID[i].Device_address, HID[i].Device_instance,
								   HID[i].touch_info.input_mode_report_id,
								   HID_REPORT_TYPE_FEATURE, touch_feature_buf, 2))
				HID[i].touch_init_step = 12; /* GET in-flight */
		}
		else if (HID[i].Device_type == USB_TOUCH && HID[i].touch_init_step == 13)
		{
			/* Write the modified Input Mode (Device Mode = 0x02, identifier
			   preserved by the GET completion). */
			if (tuh_hid_set_report(HID[i].Device_address, HID[i].Device_instance,
								   HID[i].touch_info.input_mode_report_id,
								   HID_REPORT_TYPE_FEATURE, touch_feature_buf, 2))
				HID[i].touch_init_step = 11; /* SET in-flight */
		}
		else if (HID[i].Device_type == USB_TOUCH &&
				 (HID[i].touch_init_step == 1 || HID[i].touch_init_step == 3))
		{
			/* step 1 -> cert blob, step 3 -> contact-count-max; report IDs
			   come from the descriptor. A report absent on this panel is
			   skipped (advance the step) rather than read at a guessed ID. */
			uint8_t rid = (HID[i].touch_init_step == 1)
							  ? HID[i].touch_info.cert_blob_report_id
							  : HID[i].touch_info.contact_count_max_report_id;
			if (rid == 0)
				/* absent → jump past the in-flight state: 1->3, 3->0(done) */
				HID[i].touch_init_step = (HID[i].touch_init_step == 1) ? 3 : 0;
			else if (tuh_hid_get_report(HID[i].Device_address, HID[i].Device_instance,
										rid, HID_REPORT_TYPE_FEATURE, touch_feature_buf,
										sizeof(touch_feature_buf)))
				HID[i].touch_init_step++; /* -> in-flight (2 or 4) */
		}
}

/* tuh_hid_mount_cb's part for an interface of HID_ITF_PROTOCOL_NONE: true if it is a touch screen, taken */
bool USBTouchMount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len, int slot)
{
	uint16_t vid, pid;
	tuh_vid_pid_get(dev_addr, &vid, &pid);
	HID[slot].vid = vid;
	HID[slot].pid = pid;
		/* Multi-touch USB digitizers enumerate as HID_ITF_PROTOCOL_NONE
		   with a Touch Screen application collection in their report
		   descriptor. Test for that BEFORE the gamepad VID/PID matches —
		   touch screens don't match any of those and would otherwise
		   fall through to is_generic and be misidentified as a SNES
		   gamepad. Touch is routed to slot 3 (channel 4) by default;
		   if channel 4 is already taken, fall back to whatever
		   FindFreeSlot picked. */
		{
			touch_info_t tinfo;
			bool is_digitizer = (desc_report && desc_len > 0 &&
								 analyze_touch_descriptor(desc_report, desc_len, &tinfo));
			if (is_digitizer)
			{
				int touch_slot = (HID[3].active && slot != 3) ? slot : 3;
				if (touch_slot != slot)
				{ /* the IDs went to the slot FindFreeSlot picked: they move with the device (MM.INFO(USB VID/PID n)) */
					HID[touch_slot].vid = vid;
					HID[touch_slot].pid = pid;
					HID[slot].vid = HID[slot].pid = 0;
				}
				HID[touch_slot].Device_address = dev_addr;
				HID[touch_slot].Device_instance = instance;
				HID[touch_slot].Device_type = USB_TOUCH;
				memcpy((void *)&HID[touch_slot].touch_info, &tinfo,
					   sizeof(touch_info_t));
				memset((void *)&HID[touch_slot].touch_report, 0,
					   sizeof(touch_report_t));
				/* Diagnostic: dump the descriptor + parsed layout, and arm
				   the raw-report dump for this connection. */
				dump_touch_descriptor(desc_report, desc_len, &tinfo);
				reset_touch_raw_dump();
				HID[touch_slot].report_rate = 5; /* fast polling — fingers move */
				HID[touch_slot].report_timer = -(10 + (touch_slot + 2) * 500);
				HID[touch_slot].active = true;
				HID[touch_slot].report_requested = false;
				/* Drive generic touch-panel state from this device. */
				usb_touch_present = true;
				usb_touch_active = false;
				usb_touch_active2 = false;
				/* Switch out of boot protocol so we get the multi-touch
				   report layout instead of the BIOS-mode boot-mouse
				   compatibility report (some touch screens default to
				   that for BIOS / pre-boot environments). */
				tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT);
				/* Dual-mode panels (mouse + digitizer collections) may sit
				   in mouse-compat mode until the host performs the Windows
				   touch bring-up: GET_FEATURE on the digitizer feature
				   reports. Arm that handshake — hid_app_task() issues the
				   reads, sequenced via tuh_hid_get_report_complete_cb(). It
				   runs independently of report polling, so a STALL on an
				   absent feature can't stop touch working. */
				HID[touch_slot].touch_init_step =
					tinfo.has_input_mode ? 10 : (tinfo.has_pointer_fallback ? 1 : 0);
				HID[touch_slot].touch_init_us = time_us_64();
				{
					char m[100] = "";
					strcat(m, "Multi-touch Connected on channel ");
					IntToStr(m + strlen(m), touch_slot + 1, 10);
					strcat(m, " (");
					IntToStr(m + strlen(m), tinfo.max_contacts, 10);
					strcat(m, " contacts, ");
					IntToStr(m + strlen(m), tinfo.x_logical_max, 10);
					strcat(m, "x");
					IntToStr(m + strlen(m), tinfo.y_logical_max, 10);
					strcat(m, ")\r\n> ");
					USBMessage(m);
				}
				return true;
			}
			/* Standalone absolute-pointer interface — e.g. a composite touch
			   monitor's "mouse" interface that reports touch as absolute X/Y.
			   analyze_touch_descriptor() returned false (no Finger
			   collections) but still captured the pointer layout. Route it
			   into the touch pipeline as a single-touch source: report
			   processing reuses process_pointer_report() via USB_TOUCHMOUSE. */
			/* (the CMM2: a touch monitor is one touch screen, its multi-touch one: Peter, 27 Oct 2026) */
			for (int i = 0; i < 4; i++)
				if (HID[i].active && HID[i].Device_address == dev_addr && HID[i].Device_type == USB_TOUCH)
					return false;
			if (desc_report && desc_len > 0 && tinfo.has_pointer_fallback)
			{
				tinfo.valid = true; /* pointer-only, but a usable touch source */
				HID[slot].Device_address = dev_addr;
				HID[slot].Device_instance = instance;
				HID[slot].Device_type = USB_TOUCHMOUSE;
				memcpy((void *)&HID[slot].touch_info, &tinfo, sizeof(touch_info_t));
				memset((void *)&HID[slot].touch_report, 0, sizeof(touch_report_t));
				dump_touch_descriptor(desc_report, desc_len, &tinfo);
				reset_touch_raw_dump();
				HID[slot].report_rate = 5;
				HID[slot].report_timer = -(10 + (slot + 2) * 500);
				HID[slot].active = true;
				HID[slot].report_requested = false;
				HID[slot].touch_init_step = 0; /* no digitizer handshake */
				usb_touch_present = true;
				usb_touch_active = false;
				usb_touch_active2 = false;
				tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT);
				{
					char m[100] = "";
					strcat(m, "USB Touchscreen Connected on channel ");
					IntToStr(m + strlen(m), slot + 1, 10);
					strcat(m, " (");
					IntToStr(m + strlen(m), tinfo.pointer_x_logical_max, 10);
					strcat(m, "x");
					IntToStr(m + strlen(m), tinfo.pointer_y_logical_max, 10);
					strcat(m, ")\r\n> ");
					USBMessage(m);
				}
				return true;
			}
		}
	return false;
}

/* tuh_hid_umount_cb's part for slot i */
void USBTouchUnmount(uint8_t dev_addr, uint8_t instance, int i)
{
		if (instance == HID[i].Device_instance && dev_addr == HID[i].Device_address && HID[i].Device_type == USB_TOUCH)
		{
			usb_touch_present = false;
			usb_touch_active = false;
			usb_touch_active2 = false;
			USBMessage("Multi-touch Disconnected\r\n> ");
			return;
		}
		else if (instance == HID[i].Device_instance && dev_addr == HID[i].Device_address && HID[i].Device_type == USB_TOUCHMOUSE)
		{
			usb_touch_present = false;
			usb_touch_active = false;
			usb_touch_active2 = false;
			USBMessage("USB Touchscreen Disconnected\r\n> ");
			return;
		}
}

/* tuh_hid_report_received_cb's part for slot n */
void USBTouchReport(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len, int n)
{
		if (HID[n].Device_type == USB_TOUCH || HID[n].Device_type == USB_TOUCHMOUSE)
		{
			/* Reassembles fragmented reports, then decodes + dumps the
			   complete report (the host hands up one 64-byte packet per
			   callback, so 84-byte reports arrive as 64 + tail). */
			touch_reassemble(dev_addr, instance, report, len,
							 (const touch_info_t *)&HID[n].touch_info,
							 (touch_report_t *)&HID[n].touch_report);
			/* Dump the RAW callback (actual length + full bytes) so framing
			   vs content problems are visible; decode result reflects this
			   report for the normal one-report-per-callback case. */
			dump_touch_report((const touch_info_t *)&HID[n].touch_info, report, len,
							  (const touch_report_t *)&HID[n].touch_report);
		}
}

#ifdef TOUCH_GESTURES
/* ====================================================================
 *  Touch gesture state machine.
 *  Modelled on the mouse double-click state machine in KeyboardMap.c:
 *  ProcessTouch (GUI.c) calls touch_gesture_on_down / _on_up at the
 *  edges, the helpers track the swipe in module-static state, and
 *  fun_touch reads it back via TOUCH(SWL/SWR/SWU/SWD) or TOUCH(SWIPE).
 *
 *  Direction codes:
 *    0 = none, 1 = left, 2 = right, 3 = up, 4 = down
 *
 *  Latching: a detected swipe stays latched until the next touch-down
 *  edge clears it. That lets the BASIC program poll TOUCH(SWL) etc.
 *  any time between gestures without race-of-read risk, the way
 *  nunstruct[n].Z works for the mouse double-click.
 * ==================================================================== */
static int16_t touch_swipe_start_x = 0;
static int16_t touch_swipe_start_y = 0;
static uint64_t touch_swipe_start_us = 0;
int touch_swipe_dir = 0;                   /* extern-visible via Draw.h */
int touch_tap = 0;                         /* 0 / 1, cleared on read */
int touch_longpress = 0;                   /* 0 / 1, cleared on read */
int touch_doubletap = 0;                   /* 0 / 1, cleared on read */
static bool touch_longpress_fired = false; /* set during hold; suppresses tap/swipe at lift */
static uint64_t touch_last_tap_us = 0;
static int16_t touch_last_tap_x = 0;
static int16_t touch_last_tap_y = 0;

/* Tunables — kept together so they're easy to tweak.
   TAP_MAX_DT       quick-press cap before a tap turns into a "held but
                    too short to be a long press"
   HOLD_MIN_DT      threshold below which a held finger isn't a long press
   STILL_MAX        pixel-radius the finger may wander and still count
                    as a tap / long press / double-tap second-touch
   DBL_TAP_WINDOW   max gap between two taps that get fused into a
                    double-tap
   DBL_TAP_RADIUS   max distance between the two taps' positions
   SWIPE_MAX_DT     above this dt the gesture is a slow drag, not a swipe
*/
#define TAP_MAX_DT 250000ULL
#define HOLD_MIN_DT 500000ULL
#define STILL_MAX 15
#define DBL_TAP_WINDOW 500000ULL
#define DBL_TAP_RADIUS 30
#define SWIPE_MAX_DT 600000ULL

void touch_gesture_on_down(int16_t x, int16_t y)
{
    touch_swipe_start_x = x;
    touch_swipe_start_y = y;
    touch_swipe_start_us = time_us_64();
    touch_swipe_dir = 0;
    touch_longpress_fired = false;
}

/* Called frequently while a touch is in progress (from the
   process_touch_report path for USB; could be wired into ProcessTouch
   for resistive panels too). Detects a long-press WHILE the finger is
   still down so UI code can show the menu / highlight without waiting
   for the lift. */
void touch_gesture_tick(int16_t cur_x, int16_t cur_y, bool is_down)
{
    if (!is_down)
        return;
    if (touch_swipe_start_us == 0)
        return;
    if (touch_longpress_fired)
        return;
    uint64_t dt = time_us_64() - touch_swipe_start_us;
    if (dt < HOLD_MIN_DT)
        return;
    int dx = (int)cur_x - (int)touch_swipe_start_x;
    int dy = (int)cur_y - (int)touch_swipe_start_y;
    int adx = (dx < 0) ? -dx : dx;
    int ady = (dy < 0) ? -dy : dy;
    if (adx > STILL_MAX || ady > STILL_MAX)
        return; /* moved too much — not a hold */
    touch_longpress = 1;
    touch_longpress_fired = true;
}

void touch_gesture_on_up(int16_t end_x, int16_t end_y)
{
    if (touch_swipe_start_us == 0)
        return;
    /* If long-press already fired during the hold, the touch is
       consumed — don't also report a tap or swipe for the same gesture. */
    if (touch_longpress_fired)
        return;
    uint64_t dt = time_us_64() - touch_swipe_start_us;
    int dx = end_x - touch_swipe_start_x;
    int dy = end_y - touch_swipe_start_y;
    /* Close the gesture window. Past this point only touch_swipe_start_x/y
       (the down position) are used, so zeroing start_us here makes every
       classification path below leave the machine idle — and stops a
       sampler that runs touch_gesture_tick() before the next
       touch_gesture_on_down() from seeing a stale start and mis-firing a
       long-press between gestures. */
    touch_swipe_start_us = 0;
    int adx = (dx < 0) ? -dx : dx;
    int ady = (dy < 0) ? -dy : dy;
    /* Swipe threshold ~15% of the shorter screen dimension, with a
       30-pixel floor; falls back to 30 px when HRes/VRes aren't
       initialised. */
    int min_dim = (HRes > 0 && VRes > 0)
                      ? (HRes < VRes ? HRes : VRes)
                      : 240;
    int sw_thresh = min_dim / 6;
    if (sw_thresh < 30)
        sw_thresh = 30;

    /* Try swipe first: large enough motion in the right time window. */
    if ((adx >= sw_thresh || ady >= sw_thresh) && dt <= SWIPE_MAX_DT)
    {
        if (adx > ady)
            touch_swipe_dir = (dx > 0) ? 2 : 1;
        else
            touch_swipe_dir = (dy > 0) ? 4 : 3;
        return;
    }

    /* Not a swipe. For tap / hold / double-tap classification the
       finger must have stayed roughly still. */
    if (adx > STILL_MAX || ady > STILL_MAX)
        return;

    if (dt >= HOLD_MIN_DT)
    {
        /* At-lift long-press fallback (covers callers that don't run
           the tick — resistive panels currently). */
        touch_longpress = 1;
        return;
    }
    if (dt <= TAP_MAX_DT)
    {
        /* Tap. Check whether it fuses with a previous tap to make a
           double-tap. */
        uint64_t now = time_us_64();
        if (touch_last_tap_us != 0 && (now - touch_last_tap_us) <= DBL_TAP_WINDOW)
        {
            int ddx = (int)touch_swipe_start_x - (int)touch_last_tap_x;
            int ddy = (int)touch_swipe_start_y - (int)touch_last_tap_y;
            int addx = (ddx < 0) ? -ddx : ddx;
            int addy = (ddy < 0) ? -ddy : ddy;
            if (addx <= DBL_TAP_RADIUS && addy <= DBL_TAP_RADIUS)
            {
                touch_doubletap = 1;
                touch_last_tap_us = 0; /* don't chain into triple-tap */
                return;
            }
        }
        touch_tap = 1;
        touch_last_tap_us = now;
        touch_last_tap_x = touch_swipe_start_x;
        touch_last_tap_y = touch_swipe_start_y;
    }
    /* else: between TAP_MAX_DT and HOLD_MIN_DT — not classified */
}

/* Two-finger gestures. Driven from process_touch_report when both
   contacts are simultaneously active. We capture the contact vector
   (x2-x1, y2-y1) at the moment both come down and again at the moment
   either lifts; that one vector pair is enough for all three
   classifications:
     pinch  — change in vector LENGTH (squared, to avoid sqrt)
     rotate — change in vector ANGLE (atan2 cross/dot)
     2-tap  — both contacts barely moved and the gesture was short
*/
static int16_t pinch_start_x1 = 0, pinch_start_y1 = 0;
static int16_t pinch_start_x2 = 0, pinch_start_y2 = 0;
static uint64_t pinch_start_us = 0;
static uint32_t touch_pinch_initial_dsq = 0;
int touch_pinch_dir = 0;  /* 0=none, 1=expand, 2=contract */
int touch_rotate_dir = 0; /* 0=none, 1=CW, 2=CCW */
int touch_twotap = 0;     /* 0 or 1 */

/* ~15° angular threshold for rotate. atan2 returns -π..π; 15° = ~0.26 rad. */
#define ROTATE_RAD_THRESHOLD 0.26f
/* Per-contact stillness for two-finger tap. */
#define TWOTAP_STILL_MAX 15
#define TWOTAP_MAX_DT 300000ULL

void touch_gesture_pinch_start(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    pinch_start_x1 = x1;
    pinch_start_y1 = y1;
    pinch_start_x2 = x2;
    pinch_start_y2 = y2;
    pinch_start_us = time_us_64();
    int dx = (int)x2 - (int)x1;
    int dy = (int)y2 - (int)y1;
    touch_pinch_initial_dsq = (uint32_t)(dx * dx + dy * dy);
    touch_pinch_dir = 0;
    touch_rotate_dir = 0;
    touch_twotap = 0;
}

void touch_gesture_pinch_end(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    if (touch_pinch_initial_dsq == 0)
        return;
    uint64_t dt = time_us_64() - pinch_start_us;
    int dx_init = (int)pinch_start_x2 - (int)pinch_start_x1;
    int dy_init = (int)pinch_start_y2 - (int)pinch_start_y1;
    int dx_end = (int)x2 - (int)x1;
    int dy_end = (int)y2 - (int)y1;
    uint32_t init_dsq = touch_pinch_initial_dsq;
    uint32_t final_dsq = (uint32_t)(dx_end * dx_end + dy_end * dy_end);
    touch_pinch_initial_dsq = 0;
    uint32_t change_dsq = (final_dsq > init_dsq)
                              ? (final_dsq - init_dsq)
                              : (init_dsq - final_dsq);

    /* 1. Pinch (distance ratio change ≥ 30%). */
    if (change_dsq >= 900)
    {
        if (final_dsq * 100u > init_dsq * 169u)
        {
            touch_pinch_dir = 1; /* expand */
            return;
        }
        if (final_dsq * 169u < init_dsq * 100u)
        {
            touch_pinch_dir = 2; /* contract */
            return;
        }
        /* Distance changed but ratio inconclusive — keep going. */
    }

    /* 2. Rotate (angle between contact vectors ≥ ~15°).
       cross = dx_init * dy_end - dy_init * dx_end
       dot   = dx_init * dx_end + dy_init * dy_end
       angle = atan2(cross, dot)  — radians, sign-preserving
       Screen Y is down, so positive cross = clockwise on screen. */
    float cross = (float)dx_init * (float)dy_end - (float)dy_init * (float)dx_end;
    float dot = (float)dx_init * (float)dx_end + (float)dy_init * (float)dy_end;
    if (cross != 0.0f || dot != 0.0f)
    {
        float angle = atan2f(cross, dot);
        if (angle > ROTATE_RAD_THRESHOLD)
        {
            touch_rotate_dir = 1; /* CW */
            return;
        }
        if (angle < -ROTATE_RAD_THRESHOLD)
        {
            touch_rotate_dir = 2; /* CCW */
            return;
        }
    }

    /* 3. Two-finger tap (both contacts still + short duration). */
    if (dt <= TWOTAP_MAX_DT)
    {
        int dx1 = (int)x1 - (int)pinch_start_x1;
        int dy1 = (int)y1 - (int)pinch_start_y1;
        int dx2 = (int)x2 - (int)pinch_start_x2;
        int dy2 = (int)y2 - (int)pinch_start_y2;
        int adx1 = (dx1 < 0) ? -dx1 : dx1;
        int ady1 = (dy1 < 0) ? -dy1 : dy1;
        int adx2 = (dx2 < 0) ? -dx2 : dx2;
        int ady2 = (dy2 < 0) ? -dy2 : dy2;
        if (adx1 <= TWOTAP_STILL_MAX && ady1 <= TWOTAP_STILL_MAX && adx2 <= TWOTAP_STILL_MAX && ady2 <= TWOTAP_STILL_MAX)
        {
            touch_twotap = 1;
        }
    }
}
#endif /* TOUCH_GESTURES — end of shared gesture state machine */
