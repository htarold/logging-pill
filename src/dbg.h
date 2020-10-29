#ifndef DBG_H
#define DBG_H

#ifdef DBG
#warning LED debugging ENABLED
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "tx.h"
#define dbg(x) x
#define LED_INIT \
  rcc_periph_clock_enable(RCC_GPIOC); \
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, \
  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); LED_OFF
#define LED_ON GPIO_BSRR(GPIOC) = (GPIO13<<16)
#define LED_OFF GPIO_BSRR(GPIOC) = GPIO13
#define LED_TOGGLE gpio_toggle(GPIOC, GPIO13)
#define STATIC

#else

#warning LED debugging DISABLED
#define dbg(x) /* nothing */
#define LED_INIT
#define LED_ON
#define LED_OFF
#define LED_TOGGLE
#define STATIC static

#endif

#endif /* DBG_H */
