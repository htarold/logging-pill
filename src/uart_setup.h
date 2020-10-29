#ifndef UART_SETUP_H
#define UART_SETUP_H

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>

#define uart_rxne(usart) \
  (USART_SR(usart) & USART_SR_RXNE)

#define uart_getc(usart) \
  usart_recv_blocking((usart))

#define uart_putc(usart, data) \
  usart_send_blocking((usart), (data))

extern void uart_setup(uint32_t usart,
  uint32_t port, uint32_t txbit, uint32_t bps);

#endif /* UART_SETUP_H */
