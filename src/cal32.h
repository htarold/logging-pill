#ifndef CAL32_H
#define CAL32_H
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t time_t;

/* Not in BCD */
struct tm {
  uint8_t tm_sec;                     /* 0-59 */
  uint8_t tm_min;                     /* 0-59 */
  uint8_t tm_hour;                    /* 0-23 */
  uint8_t tm_mday;                    /* 1-31 */
  uint8_t tm_mon;                     /* 0-11 */
  uint8_t tm_year;                    /* 0-99 */
};

/*
  tmp may be null, to return current time.
 */
extern time_t cal_make_time(struct tm * tmp);
/*
  Returns -1 if time is out of range.
 */
extern int8_t cal_make_cal(struct tm * tmp, time_t t);
extern bool cal_init(struct tm * tmp);
extern char * cal_print(struct tm * tmp,
  char buf[sizeof("20-12-31,19:20:21")]);
extern volatile uint8_t cal_alarmed;
extern void cal_set_alarm(time_t when, time_t period);
#endif /* CAL32_H */
