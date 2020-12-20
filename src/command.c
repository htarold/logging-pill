/*
  User interface
 */
#include "uart_setup.h"
#include "ymodem.h"
#include "cal32.h"
#include "sensors.h"
#include "msn.h"
#include "mcu.h"
#include "command.h"
#include "vdd_mv.h"
#define DBG
#include "dbg.h"

static void drain_input(void)
{
  while (uart_rxne(USART1)) uart_getc(USART1);
}
static uint8_t keystroke_timed_out(void)
{
  int i, j;

  drain_input();
  for (i = 10; ; i--) {
    if (i <= 0) return 1;
    tx_puts("Valid mission exists and will run, press any key to modify (");
    tx_putdec(i);
    tx_puts(")     \r\n");
    for (j = 100; j > 0; j--) {
      if (uart_rxne(USART1)) break;
      mcu_sleep_ms(10);
    }
    if (j) break;
  }
  drain_input();
  return 0;
}

static uint8_t clock_is_good(struct tm * tmp)
{
  if (tmp->tm_year < 20) return 0;
  if (tmp->tm_mon < 1 || tmp->tm_mon > 12) return 0;
  if (tmp->tm_mday < 1 || tmp->tm_mday > 31) return 0;
  if (tmp->tm_hour > 23) return 0;
  if (tmp->tm_min > 59) return 0;
  if (tmp->tm_sec > 59) return 0;
  return 1;
}

static int uart_echo(void)
{
  char ch;
  ch = uart_getc(USART1);
  tx_putc(ch);
  if (ch == '\r') tx_putc('\n');
  return ch;
}
static int read_input(char * buf, int size)
{
  int i;
  tx_puts("\r\nEnter max. ");
  tx_putdec(size);
  tx_puts(" characters: ");

  for (i = 0; i < size; ) {
    char ch;
    ch = uart_echo();
    if ('\r' == ch || '\n' == ch) break;
    if (127 == ch) {                  /* backspace */
      if (i > 0) i--;
    } else
      buf[i++] = ch;
  }

  return i;
}

static int read_num(void)
{
  int val;
  char ch;
  for (val = 0; ; ) {
    ch = uart_echo();
    if (ch < '0' || ch > '9') break;
    val *= 10;
    val += ch - '0';
  }
  return val;
}

static time_t read_time(struct tm * tmp)
{
  time_t t;
  int val;

  for ( ; ; tx_puts("\r\nInvalid number\r\n")) {
    tx_puts("\r\nEnter time as \"YY-MM-DD,hh:mm:ss\": ");
    val = read_num();
    if (val >= 2020) val -= 2000;
    if (val > 99 || val < 20) continue;
    tmp->tm_year = val;
    tmp->tm_mon = read_num();
    if (tmp->tm_mon < 1 || tmp->tm_mon > 12) continue;
    tmp->tm_mon--;
    tmp->tm_mday = read_num();
    if (tmp->tm_mday < 1 || tmp->tm_mday > 31) continue;
    tmp->tm_hour = read_num();
    if (tmp->tm_hour > 59) continue;
    tmp->tm_min = read_num();
    if (tmp->tm_min > 59) continue;
    tmp->tm_sec = read_num();
    if (tmp->tm_sec > 59) continue;
    t = cal_make_time(tmp);
    cal_make_cal(tmp, t);
    break;
  }
  return t;
}
static int read_interval(void)
{
  int num;
  for ( ; ; ) {
    tx_puts("\r\nEnter time interval between records, in seconds:");
    num = read_num();
    if (num >= 2) break;
    tx_puts("Interval must be at least 2 seconds\r\n");
  }
  return num;
}

static void yadd(char b) { (void)ymodem_add(b); }
static int download(void)
{
  char fn[sizeof(msn.filename)+1];
  int i;
  for (i = 0; i < sizeof(fn); i++) {
    fn[i] = msn.filename[i];
    if ((char)-1 == fn[i]) break;
  }
  fn[i] = '\0';
  tx_puts("\r\nInstruct terminal emulator to start "
          "YMODEM batch/XMODEM-CRC download...\r\n");
  if (ymodem_send(fn)) {
    tx_puts("\r\nError sending filename\r\n");
    return -1;
  }
  msn_print(yadd, 0, -1);
  ymodem_flush();
  ymodem_end();
  return 0;
}

static void view_data(uint32_t start_addr, uint32_t count)
{
  int i;
  tx_puts("\r\n");
  tx_puts("\r\n");
  tx_puts("# Data comes with note: \"");
  for (i = 0; i < sizeof(msn.filename); i++) {
    if ((char)-1 == msn.filename[i]) break;
    tx_putc(msn.filename[i]);
  }
  tx_puts("\"\r\n");
  if ((int32_t)count > 0 && start_addr > 0) tx_puts("...\r\n");
  msn_print(tx_putc, start_addr, count);
  if ((int32_t)count > 0 && (!start_addr)) tx_puts("...\r\n");
  tx_puts("# End of view\r\n\r\n");
}

void command(void)
{
  struct tm tm;
  uint8_t flag_downloaded = 0;
  int i;

  /*
    Allow modification of old mission even if valid.
   */
  cal_make_cal(&tm, cal_make_time(0));
  if (clock_is_good(&tm))
    if (0 == msn_save_params())
      if (keystroke_timed_out()) return;

  for ( ; ; ) {
    char buf[sizeof("20-12-31,23:59:59")];
    uint16_t mv;
    char cmd;
    uint32_t avail, used, total;

    tx_puts("\r\n");
    mv = vdd_mv();
    tx_puts("\tRegulator voltage           = ");
    tx_putdec(mv);
    tx_puts(" mV");
    if (mv < 3200)
      tx_puts(" WARNING: change or recharge battery");
    tx_puts("\r\n");

#define TOTAL_BYTES_PER_RECORD (msn.record_bytes + sizeof(time_t))

    total = (msn_data_limit - msn_data_start) / TOTAL_BYTES_PER_RECORD;
    avail = (msn_data_limit - msn_seek_end()) / TOTAL_BYTES_PER_RECORD;
    used = total - avail;
    tx_puts("\tMemory available (records)  : ");
    tx_putdec32(avail);
    tx_puts("\r\n");
    tx_puts("\tMemory used (records)       : ");
    tx_putdec32(used);
    tx_puts("\r\n");
    if (msn.interval_seconds != (uint16_t)-1) {
      uint32_t duration = (avail*msn.interval_seconds)/3600;
      if (duration > (24*4)) {
        tx_puts("\tLogging duration (days)     : ");
        duration /= 24;
      } else
        tx_puts("\tLogging duration (hours)    : ");
      tx_putdec32(duration);
      tx_puts("\r\n");
    }

    cal_make_cal(&tm, cal_make_time(0));
    tx_puts("\tChange current [T]ime       : ");
    tx_puts(cal_print(&tm, buf));
    tx_puts("\r\n");

    tx_puts("\tLogging to [s]tart at       : ");
    if (-1L != (long)msn.start_time) {
      cal_make_cal(&tm, msn.start_time);
      tx_puts(cal_print(&tm, buf));
    } else
      tx_puts("<none>");
    tx_puts("\r\n");

    tx_puts("\tLogging [i]nterval (seconds): ");
    if (-1 == (int)msn.interval_seconds)
      tx_puts("<none>");
    else
      tx_putdec(msn.interval_seconds);
    tx_puts("\r\n");

    tx_puts("\tLog to [f]ilename (");
    tx_putdec(sizeof(msn.filename));
    tx_puts(" chars): ");
    for (i = 0; i < sizeof(msn.filename)-1; i++) {
      if ((char)-1 == msn.filename[i]) break;
      tx_putc(msn.filename[i]);
    }
    if (0 == i)
      tx_puts("<none>");
    tx_puts("\r\n");

    if (!used)
      tx_puts("\tOR choose [r]un: ");
    else
      tx_puts("\tOR choose [r]un,/[d]ownload/"
              "view [a]ll data,[h]ead,[t]ail/[E]rase: ");
    cmd = uart_echo();
    tx_puts("\r\n");

    switch (cmd) {
    case 'T': read_time(&tm); cal_init(&tm); break;
    case 's': msn.start_time = read_time(&tm); break;
    case 'i': msn.interval_seconds = read_interval();  break;
    case 'f': read_input(msn.filename, sizeof(msn.filename)); break;
    case 'r':
      if (-1L == (long)msn.start_time)
        tx_puts("\r\nPlease fill in mission [s]tart time\r\n");
      else if (-1 == (int)msn.interval_seconds)
        tx_puts("\r\nPlease fill in logging [i]nterval\r\n");
      else if ((char)-1 == msn.filename[0])
        tx_puts("\r\nPlease fill in mission [n]otes\r\n");
      else if (msn_save_params())
        tx_puts("\r\n[E]rase first to save a new mission\r\n");
      else
        return;
      break;
    case 'd': if (0 == download()) flag_downloaded = 1; break;
    case 'a': view_all: view_data(0, -1); flag_downloaded = 1; break;
#define NR_VIEW_LINES 20
    case 't':
      if (used < NR_VIEW_LINES) goto view_all;
      view_data(msn_data_start +
                (used - NR_VIEW_LINES) * TOTAL_BYTES_PER_RECORD,
                NR_VIEW_LINES);
      flag_downloaded = 1;
      break;
    case 'h':
      if (used < NR_VIEW_LINES) goto view_all;
      view_data(0, NR_VIEW_LINES);
      flag_downloaded = 1;
      break;
    case 'E':
      if (!flag_downloaded)
        tx_puts("\r\nView or download data first.\r\n");
      else if (msn_erase())
        tx_puts("\r\nERROR:erase failed!\r\n");
      break;
    default:tx_puts("\r\nUnknown command\r\n"); break;
    }
  }
}
