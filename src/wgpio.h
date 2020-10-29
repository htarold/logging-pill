#ifndef WGPIO_H
#define WGPIO_H
/*
  GPIO set up function wrappers
 */
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

extern void wgpio_set_analog(uint32_t port, uint16_t bits);
extern void wgpio_set_float(uint32_t port, uint16_t bits);
extern void wgpio_set_pullup(uint32_t port, uint16_t bits);
extern void wgpio_set_pulldown(uint32_t port, uint16_t bits);
extern void wgpio_set_pushpull(uint32_t port, uint16_t bits);
extern void wgpio_set_opendrain(uint32_t port, uint16_t bits);
extern void wgpio_set_af_float(uint32_t port, uint8_t af, uint16_t bits);
extern void wgpio_set_af_pullup(uint32_t port, uint8_t af, uint16_t bits);
extern void wgpio_set_af_pulldown(uint32_t port, uint8_t af, uint16_t bits);
extern void wgpio_set_af_pushpull(uint32_t port, uint8_t af, uint16_t bits);
extern void wgpio_set_af_opendrain(uint32_t port, uint8_t af, uint16_t bits);
#endif /* WGPIO_H */
