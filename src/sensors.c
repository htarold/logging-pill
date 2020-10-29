/*
  Sensor methods.
 */
#include <libopencm3/stm32/flash.h>
#include "sensors.h"
#include "fmt.h"
#define DBG
#include "dbg.h"

static int8_t bosch_sensors_init(void);
static void bosch_sensors_record(uint32_t addr);
static void bosch_sensors_print(uint32_t addr);

const struct {
  int8_t (*init)(void);
  void (*record)(uint32_t address);
  void (*print)(uint32_t address);
  char const * description;
  uint8_t bytes_per_record;
  uint8_t pad[3];
} sensors[] = {
  { bosch_sensors_init,
    bosch_sensors_record,
    bosch_sensors_print,
    "BME/BMP280 sensor",
    sizeof(int16_t) + sizeof(uint32_t) },
};


#define NR_SENSORS (sizeof(sensors)/sizeof(*sensors))

#define for_each_sensor(i) for (int i = 0; i < NR_SENSORS; i++)
uint16_t sensors_bytes_per_record(void)
{
  uint16_t count = 0;
  for_each_sensor(i)
    count += sensors[i].bytes_per_record;
  return count;
}

int sensors_init(void)
{
  for_each_sensor(i)
    if (sensors[i].init()) return -1;
  return 0;
}

static void (*sensors_putc)(char);
static void sensors_puts(char * s) { while (*s) sensors_putc(*s++); }

void sensors_print(void (*putc)(char), uint32_t addr)
{
  sensors_putc = putc;
  for (int i = 0; ; putc(',')) {      /* suppress last comma */
    sensors[i++].print(addr);
    if (i >= NR_SENSORS) break;
  }
}

uint32_t sensors_record(uint32_t addr)
{
  for_each_sensor(i) {
    sensors[i].record(addr);
    addr += sensors[i].bytes_per_record;
  }
  return addr;
}

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <string.h>
#include "wi2c.h"
#include "wgpio.h"
#include "mcu.h"
#include "bosch.h"

static int8_t bosch_power_on(void)
{
  int8_t er;
  wgpio_set_pushpull(GPIOB, GPIO0|GPIO1);
  gpio_clear(GPIOB, GPIO1);           /* Ground */
  gpio_set(GPIOB, GPIO0);             /* Vdd */
  wgpio_set_af_opendrain(GPIOB, 0, GPIO10|GPIO11);
  er = wi2c_init(I2C2);
  if (er)
    dbg(tx_msg("wi2c_init returned ", er));
  mcu_sleep_ms(2);
  return er;
}
static void bosch_power_off(void)
{
  gpio_clear(GPIOB, GPIO0);
}

static struct bosch bosch;

static int8_t bosch_sensors_init(void)
{
  int8_t er;

  er = bosch_power_on();
  if (er) return -1;

  er = bosch_init(&bosch, BOSCH_ADDR_PRI);
  if (er) er = bosch_init(&bosch, BOSCH_ADDR_SEC);
  if (er)
    dbg(tx_msg("bosch_init returned ", er));
  bosch_power_off();
  return er;
}

static void bosch_sensors_record(uint32_t addr)
{
  int8_t er;

  dbg(tx_puts("Entered bosch_sensors_record\r\n"));

  do {
    int i;
    er = bosch_power_on();
    if (er) break;
    er = bosch_read_start(&bosch);
    if (er) dbg(tx_msg("bosch_read_start returned ", er));
    if (er) break;
    for (i = 0; i < 20 && bosch_is_busy(&bosch); i++)
      mcu_sleep_ms(10);
    dbg(tx_msg("Delayed ms = ", i*10));
    er = bosch_read_end(&bosch);
    if (er) dbg(tx_msg("bosch_read_end returned ", er));
    if (er) break;
  } while (0);

  bosch_power_off();

  flash_program_half_word(addr + 0, (int16_t)bosch.temperature);
  flash_program_word(addr + 2,
                     ((uint32_t)bosch.pressure & 0xffffff) |
                     (((bosch.humidity/1024)& 0xff)<<24));
}

static void bosch_sensors_print(uint32_t addr)
{
  int16_t temp;
  uint32_t press_and_rh;
  temp = *((uint16_t *)addr);
  memcpy(&press_and_rh, (void *)(addr + 2), sizeof(press_and_rh));
  sensors_puts(fmt_i16d(temp));
  sensors_putc(',');
  sensors_puts(fmt_u32d(press_and_rh & 0xffffff));
  sensors_putc(',');
  sensors_puts(fmt_u16d((press_and_rh >> 24) &0xff));
}
