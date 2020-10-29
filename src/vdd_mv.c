/*
  Return VDD in millivolts.
 */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include "mcu.h"
#include "vdd_mv.h"

uint16_t vdd_mv(void)
{
  uint8_t vrefint;
  uint16_t mv;

  rcc_periph_clock_enable(RCC_ADC1);
  adc_power_off(ADC1);
  adc_disable_scan_mode(ADC1);
  adc_set_single_conversion_mode(ADC1);
  adc_disable_external_trigger_regular(ADC1);
  adc_set_right_aligned(ADC1);
  adc_enable_temperature_sensor();

  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);
  adc_power_on(ADC1);
  mcu_sleep_ms(1);
  adc_reset_calibration(ADC1);
  adc_calibrate(ADC1);

  vrefint = ADC_CHANNEL_VREF;
  adc_set_regular_sequence(ADC1, 1, &vrefint);
  adc_disable_analog_watchdog_regular(ADC1);
  adc_power_on(ADC1);

  adc_start_conversion_direct(ADC1);
  mcu_sleep_ms(1);
  /*
    1.20V internal reference voltage
    lsb/4096 == vrefint/vdd
   */
  mv = (1200 * 4096)/adc_read_regular(ADC1);
  adc_power_off(ADC1);
  rcc_periph_clock_disable(RCC_ADC1);
  return mv;
}
