/*
  User will hit reset upon any error, little need for full
  intelligent response.
  Notes:https://gist.github.com/zonque/0ae2dc8cedbcdbd9b933
 */

#include "ymodem.h"
#include "uart_setup.h"
#include "mcu.h"
#define DBG
#include "dbg.h"

#define NUL 0x00
#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CPMEOF 0x1a
#define SUB CPMEOF

static int tmo_get(int ms)
{
  if (ms)
    for ( ; ! uart_rxne(USART1); ms -= 10) {
      if (ms < 0) return -1;
      mcu_sleep_ms(10);
    }
  return uart_getc(USART1);
}

static char ybuf[128];
static int yused;
static uint16_t ycrc;
static uint8_t ypktnum;

static int8_t ymodem_flush_(char fill);
int8_t ymodem_send(char * fn)
{
  int i, ch;

  yused = 0;
  ypktnum = 0;
  ycrc = 0x0000;

  ch = tmo_get(0);
  if (-1 == ch) return -1;
  if ('C' != ch) return -2;

  /* Send first packet, containing filename */
  for (i = 0; fn[i] > 0 && i < 128; i++)
    if (ymodem_add(fn[i])) return -1;
  if (ymodem_flush_(0)) return -1;

  ch = tmo_get(0);
  if (-1 == ch) return -1;
  if ('C' != ch) return -2;

  yused = 0;
  ycrc = 0x0000;
  /* Send next file (no next file) */
    
  return 0;
}

uint16_t ymodem_crc_update(uint16_t crc, uint8_t b)
{
  int i;
  crc ^= (b << 8);
  for (i = 8; i > 0; i--) {
    if (crc & 0x8000) {
      crc <<= 1;
      crc ^= 0x1021;
    } else
      crc <<= 1;
  }
  return crc;
}

int8_t ymodem_add(char b)
{
  ybuf[yused++] = b;

  ycrc = ymodem_crc_update(ycrc, (uint8_t)b);

  if (yused < 128) return 0;

  /* Send packet */
  for ( ; ; ) {
    int i, ch;
    tx_putc(SOH);
    tx_putc(ypktnum);
    tx_putc(255 - ypktnum);
    for (i = 0; i < 128; i++)
      tx_putc(ybuf[i]);
    tx_putc((char)(ycrc >> 8));
    tx_putc((char)(ycrc & 0xff));
    ch = tmo_get(0);
    if (ACK == ch) break;
    if (NAK != ch) return -1;
  }

  ycrc = 0x0000;
  yused = 0;
  ypktnum++;
  return 0;
}

static int8_t ymodem_flush_(char fill)
{
  int8_t er;
  if (0 == yused) return 0;
  do {
    if ((er = ymodem_add(fill)))
      return er;
  } while (yused != 0);               /* packet just sent */
  return 0;
}
int8_t ymodem_flush(void)
{
  return ymodem_flush_(CPMEOF);
}

int8_t ymodem_end(void)
{
  int ch, i;
  for ( ; ; ) {
    tx_putc(EOT);
    ch = tmo_get(0);
    if (ACK == ch) break;
    if (NAK != ch) return -1;
  }
  /* Receiver may be in batch mode, requesting another file */
  ch = tmo_get(0);
  if ('C' != ch) return -1;
  ypktnum = 0;
  for (i = 0; i < 128; i++)
    if (ymodem_add(0)) return -1;
  return 0;
}
