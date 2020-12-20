/*
  Once mission parameters are set, data collection can begin.
  Data collection could be interrupted e.g. by power failure
  etc. so data may not be continuous.
 */
#include <libopencm3/stm32/flash.h>
#include <string.h>
#include "sensors.h"
#include "cal32.h"
#include "msn.h"
#include "mcu.h"
#include "tx.h"
#define DBG
#include "dbg.h"

struct msn * msnp;
struct msn msn;
uint32_t msn_flash_limit;
uint32_t msn_data_start;
uint32_t msn_data_limit;

#ifdef DBG
static void msg32x(char * s, uint32_t u)
{
  tx_puts(s);
  tx_puthex32(u);
  tx_puts("\r\n");
}
#endif

void msn_init(uint8_t data_bytes_per_record)
{
  extern int _etext;                  /* linker symbol */
  uint32_t addr;

  addr = (uint32_t)(&_etext);
  addr += 2;
  /* Assume pages are 1k in size */
  if (addr & 1023) {
    addr &= ~1023;
    addr += 1024;
  }

  msn_data_start = addr + sizeof(*msnp);
  msnp = (struct msn *)addr;
  msn = *msnp;                        /* Invalid fields are -1 */

  msn_flash_limit = addr & ~((128*1024)-1);
  msn_flash_limit |= (128*1024);
  msn_data_limit = msn_flash_limit
                 - (sizeof(time_t) + data_bytes_per_record);
  {
    uint32_t nr;
    nr = (msn_data_limit - msn_data_start)/
         (sizeof(time_t) + data_bytes_per_record);
    msn_data_limit = (nr * (sizeof(time_t) + data_bytes_per_record)) +
                      msn_data_start;
  }

  msn.record_bytes = data_bytes_per_record;
  dbg(msg32x("         _etext = ", (uint32_t)&_etext));
  dbg(msg32x("           msnp = ", (uint32_t)msnp));
  dbg(msg32x(" msn_data_start = ", (uint32_t)msn_data_start));
  dbg(msg32x("msn_flash_limit = ", (uint32_t)msn_flash_limit));
  flash_unlock();
}

int msn_erase(void)
{
  uint32_t addr;
  addr = msn_flash_limit - 1024;
  while (addr >= (uint32_t)msnp) {    /* msnp must be erased last */
    uint32_t flags;
    tx_puts("Erasing at ");
    tx_puthex32(addr);
    tx_puts("\r");
    flash_erase_page(addr);
    flags = flash_get_status_flags();
    if (flags != FLASH_SR_EOP) return -1;
    addr -= 1024;
  }
  tx_puts("\rData memory erased.     \r\n");
  return 0;
}

int msn_save_params(void)
{
  uint16_t * src, * dst;
  int i;

  /* No new params? */
  if (-1 == msn.start_time) return -1;
  if (-1 == msn.interval_seconds) return -1;
  if (-1 == msn.filename[0]) return -1;

  /* No change in params at all (continue old mission)? */
  if (!memcmp((void *)&msn, (void *)msnp, sizeof(msn)))
    return 0;

  /* Arena is not erased? */
  dst = (uint16_t *)msnp;
  for (i = 0; i < sizeof(msn)/sizeof(uint16_t); i++)
    if (0xffff != dst[i]) return -1;  /* must delete */

  /* Can save new params */
  src = (uint16_t *)&msn;
  dst = (uint16_t *)msnp;
  for (i = 0; i < (sizeof(msn)/sizeof(uint16_t)); i++)
    flash_program_half_word((uint32_t)(dst + i), src[i]);
  return 0;
}

static uint32_t io_count(uint32_t addr)
{
  time_t t;
  memcpy(&t, (void *)addr, sizeof(t));
  if ((time_t)-1 == t) return 0;
  return sizeof(time_t) + msn.record_bytes;
}

static void (*io_print_putc)(char);   /* initialised by caller */
static uint32_t io_print_count;       /* initialised by caller */
static uint32_t io_print(uint32_t addr)
{
  char buf[sizeof("20-12-31,23:59:59")];
  time_t t;
  struct tm tm;

  if (!io_print_count) return 0;
  io_print_count--;

  memcpy(&t, (void *)addr, sizeof(t));
  if ((time_t)-1 == t) return 0;
  cal_make_cal(&tm, t);
  for (char * s = cal_print(&tm, buf); *s; s++)
    io_print_putc(*s);
  io_print_putc(',');
  sensors_print(io_print_putc, addr + sizeof(time_t));
  io_print_putc('\r');
  io_print_putc('\n');
  return sizeof(time_t) + msn.record_bytes;
}

static uint32_t io_record(uint32_t addr)
{
  time_t t;
  mcu_stop();
  mcu_wake();
  t = cal_make_time(0);
  flash_program_word(addr, t);
  sensors_record(addr + sizeof(time_t));
  return sizeof(time_t) + msn.record_bytes;
}

uint32_t msn_parse_records(uint32_t addr, uint32_t (*io)(uint32_t))
{
  uint32_t consumed, c;
  for (consumed = 0; addr < msn_data_limit; consumed += c, addr += c) {
    if (addr >= msn_data_limit) break;
    if (!(c = io(addr))) break;
  }
  return consumed;
}

uint32_t msn_seek_end(void)
{
  return msn_data_start + msn_parse_records(msn_data_start, io_count);
}

void msn_print(void (*putc)(char), uint32_t start_addr, uint32_t count)
{
  io_print_putc = putc;
  io_print_count = count;             /* -1 means all */
  if (!start_addr) start_addr = msn_data_start;
  msn_parse_records(start_addr, io_print);
}
void msn_record(void)
{
  msn_parse_records(msn_seek_end(), io_record);
}
