/*
  CPU modes
 */

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/pwr.h>
#include "uart_setup.h"
#include "mcu.h"
#define MHZ 8
#include "delay.h"
#define DBG
#include "dbg.h"

void mcu_wake(void)
{
  rcc_periph_clock_enable(RCC_AFIO);
  uart_setup(USART1, GPIOA, GPIO9, 57600);
  flash_unlock();
  LED_ON;
  dbg(tx_puts("Awake!\r\n"));
}

void mcu_stop(void)
{
  /*
    Table 14: set SLEEPDEEP bit, clear PWR_CR:PDDS, select
    PWR_CR:LPDS.
    Must also configure EXTI17 in order to wake up via RTC alarm.
   */
  dbg(tx_puts("Sleep.\r\n"));
  LED_OFF;
  delay_ms(1);                        /* sleep_ms + tx_puts = problem */
  SCB_SCR |= SCB_SCR_SLEEPDEEP;
  SCB_SCR &= ~SCB_SCR_SLEEPONEXIT;
  pwr_set_stop_mode();
  pwr_voltage_regulator_low_power_in_stop();
  __asm__("wfi");
}

static volatile int16_t ticks_remaining;
void tim2_isr(void)
{
  if (timer_get_flag(TIM2, TIM_SR_UIF)) {
    timer_clear_flag(TIM2, TIM_SR_UIF);
    ticks_remaining--;
  }
}

void mcu_sleep_ms(int16_t ms)
{
  rcc_periph_clock_enable(RCC_TIM2);
  nvic_enable_irq(NVIC_TIM2_IRQ);
  rcc_periph_reset_pulse(RST_TIM2);
  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
    TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_prescaler(TIM2, ((rcc_apb1_frequency/* *2 */)/100000)-1);
  timer_disable_preload(TIM2);
  timer_continuous_mode(TIM2);
  timer_set_period(TIM2, 100);
  timer_set_counter(TIM2, 0);
  timer_enable_irq(TIM2, TIM_DIER_UIE);
  ticks_remaining = ms;
  timer_enable_counter(TIM2);

  /*
    With timer_disable_preload(), the 1st interrupt is reached
    WITHOUT prescaling XXX and therefore is very shortr.  Thereafter
    the prescaler is active and loop is of normal length.
   */
  /*
    Execution after wfi is wonky, cpu hangs.  uart is affected.
    So we busy loop for now XXX.
   */
  while (ticks_remaining >= 0) {
    /* __asm__("wfi"); */
  }

  timer_disable_irq(TIM2, TIM_DIER_UIE);
  timer_disable_counter(TIM2);
  rcc_periph_clock_disable(RCC_TIM2);
}
