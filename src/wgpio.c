/*
  GPIO set up function wrappers
 */
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "wgpio.h"

static void enable_port(uint32_t port)
{
  enum rcc_periph_clken clk;
  switch (port) {
#ifdef GPIOA
  case GPIOA: clk = RCC_GPIOA; break;
#endif
#ifdef GPIOB
  case GPIOB: clk = RCC_GPIOB; break;
#endif
#ifdef GPIOC
  case GPIOC: clk = RCC_GPIOC; break;
#endif
#ifdef GPIOD
  case GPIOD: clk = RCC_GPIOD; break;
#endif
#ifdef GPIOE
  case GPIOE: clk = RCC_GPIOE; break;
#endif
#ifdef GPIOF
  case GPIOF: clk = RCC_GPIOF; break;
#endif
#ifdef GPIOG
  case GPIOG: clk = RCC_GPIOG; break;
#endif
  default: for ( ; ; ) ;
  }
  rcc_periph_clock_enable(clk);
}
#ifdef RCC_APB2ENR_AFIO
static void enable_afio(void) { rcc_periph_clock_enable(RCC_AFIO); }
#else
#define enable_afio() /* nothing */
#endif

#ifdef STM32F1
void wgpio_set_analog(uint32_t port, uint16_t bits)
{
  gpio_set_mode(port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, bits);
}
void wgpio_set_float(uint32_t port, uint16_t bits)
{
  gpio_set_mode(port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, bits);
}
void wgpio_set_pullup(uint32_t port, uint16_t bits)
{
  gpio_set_mode(port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, bits);
  GPIO_ODR(port) |= bits;
}
void wgpio_set_pulldown(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_set_mode(port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, bits);
  GPIO_ODR(port) &= ~bits;
}
void wgpio_set_pushpull(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_set_mode(port, GPIO_MODE_OUTPUT_50_MHZ,
    GPIO_CNF_OUTPUT_PUSHPULL, bits);
}
void wgpio_set_opendrain(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_set_mode(port, GPIO_MODE_OUTPUT_50_MHZ,
    GPIO_CNF_OUTPUT_OPENDRAIN, bits);
}
void wgpio_set_af_float(uint32_t port, uint8_t unused, uint16_t bits)
{
  enable_afio();
  wgpio_set_float(port, bits);
}
void wgpio_set_af_pullup(uint32_t port, uint8_t unused, uint16_t bits)
{
  enable_afio();
  wgpio_set_float(port, bits);
  wgpio_set_pullup(port, bits);
}
void wgpio_set_af_pulldown(uint32_t port, uint8_t unused, uint16_t bits)
{
  enable_afio();
  wgpio_set_float(port, bits);
  wgpio_set_pulldown(port, bits);
}
void wgpio_set_af_pushpull(uint32_t port, uint8_t unused, uint16_t bits)
{
  enable_afio();
  enable_port(port);
  wgpio_set_float(port, bits);
  gpio_set_mode(port, GPIO_MODE_OUTPUT_50_MHZ,
    GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, bits);
}
void wgpio_set_af_opendrain(uint32_t port, uint8_t unused, uint16_t bits)
{
  enable_afio();
  enable_port(port);
  gpio_set_mode(port, GPIO_MODE_OUTPUT_50_MHZ,
    GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, bits);
}
#else
void wgpio_set_analog(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_mode_setup(port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, bits);
}
void wgpio_set_float(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_mode_setup(port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, bits);
}
void wgpio_set_pullup(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_mode_setup(port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, bits);
}
void wgpio_set_pulldown(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_mode_setup(port, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, bits);
}
void wgpio_set_pushpull(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_mode_setup(port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, bits);
  gpio_set_output_options(port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, bits);
}
void wgpio_set_opendrain(uint32_t port, uint16_t bits)
{
  enable_port(port);
  gpio_mode_setup(port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, bits);
  gpio_set_output_options(port, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, bits);
}
void wgpio_set_af_pullup(uint32_t port, uint8_t af, uint16_t bits)
{
  enable_afio();
  wgpio_set_pullup(port, bits);
  gpio_set_af(port, af, bits);
}
void wgpio_set_af_pulldown(uint32_t port, uint8_t af, uint16_t bits)
{
  enable_afio();
  wgpio_set_pulldown(port, bits);
  gpio_set_af(port, af, bits);
}
void wgpio_set_af_pushpull(uint32_t port, uint8_t af, uint16_t bits)
{
  enable_afio();
  wgpio_set_pushpull(port, bits);
  gpio_set_af(port, af, bits);
}
void wgpio_set_af_opendrain(uint32_t port, uint8_t af, uint16_t bits)
{
  enable_afio();
  enable_port(port);
  wgpio_set_opendrain(port, bits);
  gpio_set_af(port, af, bits);
}
#endif
