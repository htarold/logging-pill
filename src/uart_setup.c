/*
  Not for stm32f0.
 */
#include "uart_setup.h"
#include "wgpio.h"

void uart_setup(uint32_t usart, uint32_t port, uint32_t txbit, uint32_t bps)
{
  uint32_t c;
  c = 0;
  if (0) ;
#ifdef USART1
  else if (usart == USART1) c = RCC_USART1;
#endif
#ifdef USART2
  else if (usart == USART2) c = RCC_USART2;
#endif
#ifdef USART3
  else if (usart == USART3) c = RCC_USART3;
#endif
#ifdef USART4
  else if (usart == USART4) c = RCC_USART4;
#endif
#ifdef USART5
  else if (usart == USART5) c = RCC_USART5;
#endif
#ifdef USART6
  else if (usart == USART6) c = RCC_USART6;
#endif
  while (!c) ;
  rcc_periph_clock_enable(c);
  wgpio_set_af_pushpull(port, 0, txbit);
  wgpio_set_af_float(port, 0, txbit<<1);
  usart_set_baudrate(usart, bps);
  usart_set_databits(usart, 8);
  usart_set_parity(usart, USART_PARITY_NONE);
  usart_set_stopbits(usart, USART_CR2_STOPBITS_1);
  usart_set_mode(usart, USART_MODE_TX_RX);
  usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);
  usart_enable(usart);
}
