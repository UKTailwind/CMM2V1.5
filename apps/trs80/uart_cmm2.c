/* uart_cmm2.c: sdltrs's trs_uart.c for the Colour Maximite 2 (Peter, 11 Nov 2026): the TRS-80's RS-232 port on one of the CMM2's serial ports, by
   the loader's com_ services, where trs_uart.c has a Unix tty. The emulation of the UART's registers is trs_uart.c's, kept as it is (Tim Mann's,
   2000-2008, with Mark Grebe's and Jens Guenther's changes: sdltrs's licence); only the calls on the tty are replaced. sdltrs's serial setting
   (-serial, or its menu) names the port: COM1 or COM2 (the I/O connector's), or COM4 to COM7 (a USB serial device); empty, the default, is no
   serial port. The word is 7 or 8 bits (5 and 6 are sent as 8); a break is not sent. */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "cmm2.h"
#include "error.h"
#include "trs.h"
#include "trs_uart.h"
#include "trs_state_save.h"

#define BUFSIZE 256

char trs_uart_name[FILENAME_MAX] = "";

int trs_uart_switches =
  0x7 | TRS_UART_NOPAR | TRS_UART_WORD8; /* Default: 9600 8N1 */

static int initialized;

static struct {
  int modem;
  int switches;
  int baud;
  int status;
  int control;
  int idata;
  int odata;

  Uint8 buf[BUFSIZE];
  Uint8* bufp;
  int bufleft;
  int tstates;

  int com;    /* the CMM2's port, 0 if none */
  int open;
} uart;

static int trs_uart_wordbits[] = TRS_UART_WORDBITS_TABLE;
static float trs_uart_baud[] = TRS_UART_BAUD_TABLE;

/* the port opened again with the settings the TRS-80 last wrote (the tty's tcsetattr) */
static void uart_set(void)
{
  int bits, parity;

  if (uart.com == 0 || uart.baud < 0 || uart.control < 0) return;
  if (uart.open) cmm2->com_close(uart.com);
  bits = trs_uart_wordbits[TRS_UART_WORDBITS(uart.control)] == 7 ? 7 : 8;
  parity = (uart.control & TRS_UART_NOPAR) ? 0 : (uart.control & TRS_UART_EVENPAR) ? 1 : 2;
  uart.open = cmm2->com_open(uart.com, (uint32_t)(trs_uart_baud[TRS_UART_SNDBAUD(uart.baud)] + 0.5f), bits, parity,
                             (uart.control & TRS_UART_STOP2) ? 2 : 1, 4096) >= 0;
  if (!uart.open)
    error("set UART attributes: '%s': %s", trs_uart_name, cmm2->io_error());
}

void
trs_uart_init(void)
{
  if (initialized == 1 && uart.open) cmm2->com_close(uart.com);
  uart.open = 0;
  uart.com = 0;
  if (trs_uart_name[0] == '\0') {
    /* Emulate having no serial port */
    initialized = -1;
    return;
  }
  if (strncasecmp(trs_uart_name, "COM", 3) == 0 && trs_uart_name[3] >= '1' && trs_uart_name[3] <= '7' && trs_uart_name[3] != '3'
      && trs_uart_name[4] == '\0')
    uart.com = trs_uart_name[3] - '0';
  if (uart.com == 0) {
    error("open UART: '%s': the serial ports are COM1, COM2 and COM4 to COM7", trs_uart_name);
    initialized = -1;
    return;
  }
  initialized = 1;

  /* Not readable from a user process on unix */
  uart.modem = TRS_UART_CTS | TRS_UART_DSR | TRS_UART_CD;

  uart.switches = (trs_model == 1) ? trs_uart_switches : 0xff;

  /* arbitrary default */
  uart.baud = -1;
  uart.control = -1;
  trs_uart_baud_out((TRS_UART_9600 << 4) + TRS_UART_9600);

  /* arbitrary default */
  trs_uart_control_out(TRS_UART_NOPAR | TRS_UART_WORD8 | TRS_UART_NOTBREAK |
		       TRS_UART_DTR | TRS_UART_RTS);

  uart.status = TRS_UART_SENT;
  trs_uart_snd_interrupt(1);

  uart.bufp = uart.buf;
  uart.bufleft = 0;
}

int
trs_uart_modem_in(void)
{
  /* should poll hardware here, if we could */
  if (initialized == 0) trs_uart_init();
  if (initialized == -1) return 0xff;

  return uart.modem;
}

void
trs_uart_reset_out(void)
{
  if (initialized == 0) trs_uart_init();
  if (initialized == -1) {
    if (trs_uart_name[0] != '\0')
      error("serial port emulation is not enabled");
    return;
  }
}

int
trs_uart_switches_in(void)
{
  if (initialized == 0) trs_uart_init();
  if (initialized == -1) return 0xff;

  return uart.switches;
}

void
trs_uart_baud_out(int value)
{
  int bits, control;

  if (initialized == 1 && uart.baud == value) return;
  if (initialized == 0) trs_uart_init();
  if (initialized == -1) return;
  uart.baud = value;

  control = uart.control < 0 ? (TRS_UART_NOPAR | TRS_UART_WORD8) : uart.control;
  bits = 1 + trs_uart_wordbits[TRS_UART_WORDBITS(control)] +
    ((control & TRS_UART_NOPAR) ? 0 : 1) +
    ((control & TRS_UART_STOP2) ? 2 : 1);
  uart.tstates = (z80_state.clockMHz * 1000000.0 * bits)
    / trs_uart_baud[TRS_UART_SNDBAUD(value)];

  uart_set();
}

void
trs_uart_set_avail(int dummy)
{
  uart.status |= TRS_UART_RCVD;
  trs_uart_rcv_interrupt(1);
}

void
trs_uart_set_empty(int dummy)
{
  uart.status |= TRS_UART_SENT;
  trs_uart_snd_interrupt(1);
}

int
trs_uart_check_avail(void)
{
  if (initialized == 1 && uart.bufleft == 0 && uart.open) {
    /* check for data available */
    int rc = cmm2->com_read(uart.com, uart.buf, BUFSIZE);

    if (rc < 0) rc = 0;
    uart.bufp = uart.buf;
    uart.bufleft = rc;
    if (rc > 0) {
      /* be sure events don't happen too fast */
      trs_schedule_event(trs_uart_set_avail, 1, uart.tstates);
    }
  }

  return uart.bufleft;
}

int
trs_uart_status_in(void)
{
  if (initialized == 0) trs_uart_init();
  if (initialized == -1) return 0xff;
  trs_uart_check_avail();

  return uart.status;
}

void
trs_uart_control_out(int value)
{
  if (initialized == 1 && uart.control == value) return;
  if (initialized == 0) trs_uart_init();
  if (initialized == -1) return;
  uart.control = value;
  uart_set();
}

int
trs_uart_data_in(void)
{
  if (initialized == 0) trs_uart_init();
  if (initialized == -1) return 0xff;
  trs_uart_check_avail();
  if (uart.status & TRS_UART_RCVD) {
    uart.status &= ~TRS_UART_RCVD;
    trs_uart_rcv_interrupt(0);
    uart.bufleft--;
    uart.idata = *uart.bufp++;
    if (uart.bufleft) {
      trs_schedule_event(trs_uart_set_avail, 1, uart.tstates);
    }
  }

  return uart.idata;
}

void
trs_uart_data_out(int value)
{
  Uint8 c = (Uint8)value;

  if (initialized == 0) trs_uart_init();
  if (initialized == -1) return;
  uart.odata = value;
  if (uart.open)
    cmm2->com_write(uart.com, &c, 1);   /* (queued: the UART stays ready to send, as trs_uart.c leaves it after a write) */
}

void trs_uart_save(FILE *file)
{
  int inited = 0;

  trs_save_filename(file, trs_uart_name);
  trs_save_int(file, &trs_uart_switches, 1);
  trs_save_int(file, &inited, 1);
}

void trs_uart_load(FILE *file)
{
  trs_load_filename(file, trs_uart_name);
  trs_load_int(file, &trs_uart_switches, 1);
  trs_load_int(file, &initialized, 1);
}
