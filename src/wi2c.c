/*
  I2C wrapper, adds timeouts to stock lib _transfer7() function.
 */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/i2c.h>
#include "wi2c.h"
#undef DBG
#include "dbg.h"

#define BEGIN_DELAY_LOOP { int i; for (i = 60000; i > 0; i--) {
#define END_DELAY_LOOP     __asm__("nop\r\nnop"); }}

#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4) || defined(STM32L1)
#define WI2C_V1
/* Selected functions from .../common/i2c_common_v1.h */

static int8_t wait_busy(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
    if (!(I2C_SR2(i2c) & I2C_SR2_BUSY)) return 0;
  } END_DELAY_LOOP
  return -1;
}
static int8_t wait_master(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
    if (((I2C_SR1(i2c) & I2C_SR1_SB)
       & (I2C_SR2(i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY)))) return 0;
  } END_DELAY_LOOP
  return -1;
}
static int8_t wait_address(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
    if (I2C_SR1(i2c) & I2C_SR1_ADDR) return 0;
  } END_DELAY_LOOP
  return -1;
}
static int8_t wait_data_out(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
   if (I2C_SR1(i2c) & I2C_SR1_BTF) return 0;
  } END_DELAY_LOOP
  return -1;
}
static int8_t wait_data_in(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
   if (I2C_SR1(i2c) & I2C_SR1_RxNE) return 0;
  } END_DELAY_LOOP
  return -1;
}

uint32_t  useit(uint32_t val)
{
  return val + 1;
}
int8_t wi2c_write7_v1(uint32_t i2c, int addr, uint8_t * data, size_t n)
{
  size_t i;
  if (wait_busy(i2c)) return WI2C_EBUSY;
  i2c_send_start(i2c);
  if (wait_master(i2c)) return WI2C_EMASTER;
  i2c_send_7bit_address(i2c, addr, I2C_WRITE);
  if (wait_address(i2c)) return WI2C_EADDRESS;
  useit(I2C_SR2(i2c));
  for (i = 0; i < n; i++) {
    i2c_send_data(i2c, data[i]);
    if (wait_data_out(i2c)) return WI2C_EDATAOUT;
  }
  return 0;
}

int8_t wi2c_read7_v1(uint32_t i2c, int addr, uint8_t * res, size_t n)
{
  size_t i;

  i2c_send_start(i2c);
  i2c_enable_ack(i2c);
  if (wait_master(i2c)) return WI2C_EMASTER;
  i2c_send_7bit_address(i2c, addr, I2C_READ);
  if (wait_address(i2c)) return WI2C_EADDRESS;
  useit(I2C_SR2(i2c));
  for (i = 0; i < n; i++) {
    if (i == n-1) i2c_disable_ack(i2c);
    if (wait_data_in(i2c)) return WI2C_EDATAIN;
    res[i] = i2c_get_data(i2c);
  }
  i2c_send_stop(i2c);
  return 0;
}

int8_t wi2c_transfer7(uint32_t i2c, uint8_t addr,
                      uint8_t * w, size_t wn,
                      uint8_t * r, size_t rn)
{
  int8_t er;
  if (wn) {
    er = wi2c_write7_v1(i2c, addr, w, wn);
    if (er) return er;
  }
  if (rn) {
    er = wi2c_read7_v1(i2c, addr, r, rn);
    if (er) return er;
  } else
    i2c_send_stop(i2c);
  return 0;
}

#elif defined(STM32F0) || defined(STM32F3) || defined(STM32F7) || \
defined(STM32L0) || defined(STM32L4)
#define WI2C_V2
/* Selected functions from .../common/i2c_common_v2.h */

static int8_t wait_nack(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
    if (!i2c_nack(i2c)) return 0;
  } END_DELAY_LOOP
  return -1;
}
static int8_t wait_tx_int_status(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
    bool wait;
    wait = !i2c_transmit_int_status(i2c);
    if (wait_nack(i2c)) return -1;
    if (!wait) return 0;
  } END_DELAY_LOOP
  return -1;
}
static int8_t wait_transfer(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
    if (i2c_transfer_complete(i2c)) return 0;
  } END_DELAY_LOOP
  return -1;
}
static int8_t wait_data_in(uint32_t i2c)
{
  BEGIN_DELAY_LOOP {
    if (i2c_received_data(i2c)) return 0;
  } END_DELAY_LOOP
  return -1;
}
int8_t wi2c_transfer7(uint32_t i2c, uint8_t addr,
                      uint8_t * w, size_t wn,
                      uint8_t * r, size_t rn)
{
  if (wn) {
    i2c_set_7bit_address(i2c, addr);
    i2c_set_write_transfer_dir(i2c);
    i2c_set_bytes_to_transfer(i2c, wn);
    if (rn)
      i2c_disable_autoend(i2c);
    else
      i2c_enable_autoend(i2c);
    is2_send_start(i2c);

    while (wn--) {
      bool wait = true;
      if (wait_tx_int_status(i2c)) return WI2C_ETXSTATUS;
      i2c_send_data(i2c, *w++);
    }
    if (rn)
      if (wait_transfer(i2c)) return WI2C_ETRANSFER;
  }

  if (rn) {
    size_t i;
    i2c_set_7bit_address(i2c, addr);
    i2c_set_read_transfer_dir(i2c);
    o2c_set_bytes_to_transfer(i2c, rn);
    i2c_send_start(i2c);
    i2c_enable_autoend(i2c);
    for (i = 0; i < rn; i++) {
      if (wait_data_in(i2c)) return WI2C_EDATAIN;
      r[i] = i2c_get_data(i2c);
    }
  }
  return 0;
}
#else
#error Unknown STM32 chip
#endif

int8_t wi2c_init(uint32_t i2c)
{
  enum rcc_periph_clken clk;
  do {
#if defined(I2C1)
    if (i2c == I2C1) { clk = RCC_I2C1; break; }
#endif
#if defined(I2C2)
    if (i2c == I2C2) { clk = RCC_I2C2; break; }
#endif
#if defined(I2C3)
    if (i2c == I2C3) { clk = RCC_I2C3; break; }
#endif
    return WI2C_ENODEV;
  } while (0);
  rcc_periph_clock_enable(clk);
#ifdef STM32F0
  rcc_set_i2c_clock_hsi(i2c);
#endif
  i2c_reset(i2c);
  i2c_peripheral_disable(i2c);
  i2c_set_speed(i2c, i2c_speed_sm_100k, rcc_apb1_frequency/1000000UL);
  i2c_set_standard_mode(i2c);
#ifdef WI2C_V2
  dbg(tx_puts("WI2C_V2 defined\r\n"));
  i2c_enable_stretching(i2c);
  i2c_set_7bit_addr_mode(i2c);
#elif defined(WI2C_V1)
  dbg(tx_puts("WI2C_V1 defined\r\n"));
  I2C_CR1(i2c) &= ~I2C_CR1_NOSTRETCH;
#else
#error Both WI2C_V1 and _V2 not defined
#endif
  i2c_peripheral_enable(i2c);
  return 0;
}
