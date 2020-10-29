#ifndef MSN_H
#define MSN_H
/*
  Once mission parameters are set, data collection can begin.
  Data collection could be interrupted e.g. by power failure
  etc. so data may not be continuous.
 */
#include <stdint.h>
#include "cal32.h"                    /* only for time_t */

struct msn {
  time_t start_time;
  int16_t interval_seconds;
  uint8_t record_bytes;
  char filename[58];
};
/*
  Data records follow immediately after.  Each record begins
  with a time_t, then record_bytes of data.
 */
#define MSN_IO_WRITE 1

extern struct msn * msnp;
extern struct msn msn;
extern uint32_t msn_flash_limit;
extern uint32_t msn_data_start;
extern uint32_t msn_data_limit;
extern void msn_init(uint8_t data_bytes_per_record);
extern int msn_erase(void);
extern int msn_save_params(void);
extern uint32_t msn_seek_end(void);
extern uint32_t msn_parse_records(uint32_t addr, uint32_t (*io)(uint32_t));
extern void msn_print(void (*putc)(char),
  uint32_t start_addr, uint32_t count);
extern void msn_record(void);

#endif /* MSN_H */
