/*
  Calendar based on blue pill's RTC, which is a bare 32-bit counter.
  Time is based on Epoch of 00 (realistically year 2000) since
  century years are not kept.
 */

#include <libopencm3/stm32/rtc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include "cal32.h"
#define DBG
#include "dbg.h"

static const uint8_t monthdays[12] = {
31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static inline bool isleap(uint8_t year)
{
  return !(year % 4);                 /* Won't hold for 2100 XXX */
}

time_t cal_make_time(struct tm * tmp)
{
  uint8_t i;
  time_t t;

  dbg(tx_puts("Entered cal_make_time\r\n"));
  if (!tmp)
    return(rtc_get_counter_val());

  t = 0;
  for (i = 0; i < tmp->tm_year; i++) {
    t += 365*86400;
    if (isleap(i)) t += 86400;
  }
  for (i = 0; i < tmp->tm_mon; i++) {
    t += 86400 * monthdays[i];
    if (i == 1 && isleap(tmp->tm_year))
      t += 86400;
  }
  t += (tmp->tm_mday - 1) * 24 * 60 * 60;
  t += tmp->tm_hour * 60 * 60;
  t += tmp->tm_min * 60;
  t += tmp->tm_sec;
  dbg(tx_puts("Returning from cal_make_time\r\n"));
  return(t);
}

int8_t cal_make_cal(struct tm * tmp, time_t t)
{
  time_t now;
  uint32_t inc;

  now = 0;
  for (tmp->tm_year = 0; now < t; tmp->tm_year++) {
    if (tmp->tm_year >= 99) return(-1);
    inc = 365 * 86400;
    if (isleap(tmp->tm_year)) inc += 86400;
    if (now + inc > t) break;
    now += inc;
  }

  tmp->tm_year %= 100;

  for (tmp->tm_mon = 0; now < t; tmp->tm_mon++) {
    inc = monthdays[tmp->tm_mon] * 86400;
    if (1 == tmp->tm_mon)
      if (isleap(tmp->tm_year))
        inc += 86400;
    if (now + inc > t) break;
    now += inc;
  }

  t -= now;
  tmp->tm_mday = t/86400;
  t -= tmp->tm_mday * 86400;
  tmp->tm_mday++;                     /* Starts from 1 */

  tmp->tm_hour = t/(60*60);
  t -= tmp->tm_hour * 60 * 60;

  tmp->tm_min = t / 60;
  t -= tmp->tm_min * 60;

  tmp->tm_sec = t;
  return(0);
}

static void print_field(char * p, uint8_t val, char sep)
{
  p[0] = (val/10) + '0';
  p[1] = (val%10) + '0';
  p[2] = sep;
}
char * cal_print(struct tm * tmp, char buf[sizeof("20-12-31,19:20:21")])
{
  char * start;
  start = buf;
  print_field(buf, tmp->tm_year, '-');
  print_field(buf += 3, tmp->tm_mon, '-');
  print_field(buf += 3, tmp->tm_mday, ',');
  print_field(buf += 3, tmp->tm_hour, ':');
  print_field(buf += 3, tmp->tm_min, ':');
  print_field(buf += 3, tmp->tm_sec, '\0');
  return start;
}

volatile uint8_t cal_alarmed;
static uint16_t alarm_period;
static time_t alarm_time;
static void isr_body(void)
{
  cal_alarmed++;
  if (alarm_period)
    rtc_set_alarm_time(alarm_time += alarm_period);
}
void rtc_isr(void) { rtc_clear_flag(RTC_ALR); isr_body(); }
void rtc_alarm_isr(void) { exti_reset_request(1<<17); }

#if 1
static void rtc2_awake_from_off(enum rcc_osc clock_source)
{
        uint32_t reg32;

        /* Enable power and backup interface clocks. */
        rcc_periph_clock_enable(RCC_PWR);
        rcc_periph_clock_enable(RCC_BKP);

        /* Enable access to the backup registers and the RTC. */
        dbg(tx_puts("cal_init calling pwr_disable_backup_domain_write_protect\r\n"));
        pwr_disable_backup_domain_write_protect();

        /* Set the clock source */
        dbg(tx_puts("cal_init calling rcc_set_rtc_clock_source\r\n"));
        rcc_set_rtc_clock_source(clock_source);

        /* Clear the RTC Control Register */
        RTC_CRH = 0;
        RTC_CRL = 0;

        /* Enable the RTC. */
        dbg(tx_puts("cal_init calling rcc_enable_rtc_clock\r\n"));
        rcc_enable_rtc_clock();

        /* Clear the Registers */
        dbg(tx_puts("cal_init calling rtc_enter_config_mode\r\n"));
        rtc_enter_config_mode();
        RTC_PRLH = 0;
        RTC_PRLL = 0;
        RTC_CNTH = 0;
        RTC_CNTL = 0;
        RTC_ALRH = 0xFFFF;
        RTC_ALRL = 0xFFFF;
        dbg(tx_puts("cal_init calling rtc_exit_config_mode\r\n"));
        rtc_exit_config_mode();

        /* Wait for the RSF bit in RTC_CRL to be set by hardware. */
        RTC_CRL &= ~RTC_CRL_RSF;
        dbg(tx_puts("cal_init waiting for RTC_CRL_RSF\r\n"));
        while ((reg32 = (RTC_CRL & RTC_CRL_RSF)) == 0);
}
#else
#define rtc2_awake_from_off(x) rtc_awake_from_off(x)
#endif /* 0 */
bool cal_init(struct tm * tmp)
{
  bool already_running;
  dbg(tx_puts("Entered cal_init\r\n"));

  rcc_periph_clock_enable(RCC_PWR);
  rcc_periph_clock_enable(RCC_BKP);

  already_running = (bool)rcc_rtc_clock_enabled_flag();
  if (already_running) {
    dbg(tx_puts("cal_init calling rtc_awake_from_standby\r\n"));
    rtc_awake_from_standby();
  } else {
    dbg(tx_puts("cal_init calling rtc_awake_from_off\r\n"));
    rtc2_awake_from_off(RCC_LSE);
    rtc_set_prescale_val(0x7fff);
  }

  if (tmp) {
    dbg(tx_puts("cal_init calling rtc_set_counter_val\r\n"));
    rtc_set_counter_val(tmp?cal_make_time(tmp):0);
  }

  cal_alarmed = 0;
  dbg(tx_puts("cal_init calling nvic_enable_irq\r\n"));
  nvic_enable_irq(NVIC_RTC_IRQ);
  dbg(tx_puts("cal_init calling nvic_set_priority\r\n"));
  nvic_set_priority(NVIC_RTC_IRQ, 1);
  dbg(tx_puts("cal_init calling rtc_interrupt_enable\r\n"));
  rtc_interrupt_enable(RTC_ALR);      /* or RTC_SEC, or RTC_OW */
  dbg(tx_puts("Returning from cal_init\r\n"));

  return(already_running);
}

void cal_set_alarm(time_t when, time_t period)
{
  if (!when) {
    rtc_disable_alarm();
    alarm_period = 0;
    return;
  }
  alarm_period = period;              /* 0 means just once */
  rtc_set_alarm_time(alarm_time = when);
  rtc_enable_alarm();
}
