/*
  
 */
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include "uart_setup.h"
#include "cal32.h"
#include "sensors.h"
#include "msn.h"
#include "command.h"
#include "mcu.h"
#define DBG
#include "dbg.h"
#define MHZ 8
#include "delay.h"

static void die(char * msg, int16_t er)
{
  tx_msg(msg, er);
  for ( ; ; )
    mcu_sleep_ms(10);
  /* Not Reached */
}

void initialise(void)
{
  if (sensors_init())
    die("FATAL:sensors_init failed ", -1);
  tx_puts("Sensors initialised and tested ok\r\n");
  msn_init(sensors_bytes_per_record());
  tx_puts("Checking real time clock...\r\n");
  cal_init(0);
  tx_puts("Real time clock OK\r\n");
  exti_enable_request(EXTI17);
  exti_set_trigger(EXTI17, EXTI_TRIGGER_RISING);
  nvic_enable_irq(NVIC_RTC_ALARM_IRQ);
  nvic_set_priority(NVIC_RTC_ALARM_IRQ, 2);
}

int
main(void)
{
  time_t when;

  LED_INIT;
  mcu_wake();

  for (int i = 4; i > 0; i--) {
    tx_msg("Wait... ", i);
    mcu_sleep_ms(1000);
  }

  initialise();
  command();

  when = cal_make_time(0);
  if (when < msn.start_time)
    when = msn.start_time;
  when /= msn.interval_seconds;
  when *= msn.interval_seconds;
  when += msn.interval_seconds;
  cal_set_alarm(when, msn.interval_seconds);

  tx_puts("\r\nNext log record at ");
  {
    struct tm tm;
    char buf[sizeof("20-12-31,23:59:59")];
    cal_make_cal(&tm, when);
    tx_puts(cal_print(&tm, buf));
  }
  tx_puts("\r\nVerify that the green LED goes out.\r\n");

  msn_record();

  for ( ; ; ) {
    tx_puts("Out of memory\r\n");
    mcu_stop();
    mcu_wake();
  }
}
