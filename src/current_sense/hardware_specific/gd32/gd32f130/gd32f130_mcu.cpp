#include "../../../hardware_api.h"

#if defined(GD32F1x0)
#include "../../../../common/foc_utils.h"
#include "../../../../drivers/hardware_api.h"
#include "../../../../drivers/hardware_specific/gd32/gd32_mcu.h"
#include "../../../hardware_api.h"
#include "../gd32_mcu.h"
#include "gd32/PinNames.h"
#include "Arduino.h"

#define _ADC_VOLTAGE_F1 3.3f
#define _ADC_RESOLUTION_F1 4096.0f
#define SAMPLING_TIME ADC_SAMPLETIME_1POINT5

#ifdef SIMPLEFOC_GD32_ADC_INTERRUPT
// values of the 3 injected channels, stored by the ADC interrupt
static volatile uint32_t adc_val[3] = {0};
#endif

void* _configureADCLowSide(const void* driver_params, const int pinA, const int pinB, const int pinC){

  GD32CurrentSenseParams* cs_params= new GD32CurrentSenseParams {
    .pins={(int)NOT_SET,(int)NOT_SET,(int)NOT_SET},
    .adc_voltage_conv = (_ADC_VOLTAGE_F1) / (_ADC_RESOLUTION_F1)
  };

  uint8_t cnt = 0;
  if(_isset(pinA)){
    pinMode(pinA,INPUT_ANALOG);
    cs_params->pins[cnt++] = pinA;
  }
  if(_isset(pinB)){
    pinMode(pinB,INPUT_ANALOG);
    cs_params->pins[cnt++] = pinB;
  }
  if(_isset(pinC)){
    pinMode(pinC,INPUT_ANALOG);
    cs_params->pins[cnt] = pinC;
  }

  // Enable ADC and DMA clock
  rcu_periph_clock_enable(RCU_ADC);

  // Configure ADC clock (APB2 clock is DIV1 -> 72MHz, ADC clock is DIV6 -> 12MHz)
  rcu_adc_clock_config(RCU_ADCCK_APB2_DIV6);

  adc_channel_length_config(ADC_INSERTED_CHANNEL, _isset(cs_params->pins[2]) ? 3 : 2);
  adc_inserted_channel_config(0,get_adc_channel(DIGITAL_TO_PINNAME(cs_params->pins[0])),SAMPLING_TIME);
  adc_inserted_channel_config(1,get_adc_channel(DIGITAL_TO_PINNAME(cs_params->pins[1])),SAMPLING_TIME);
  if(_isset(cs_params->pins[2])){
    adc_inserted_channel_config(2,get_adc_channel(DIGITAL_TO_PINNAME(cs_params->pins[2])),SAMPLING_TIME);
  }
  adc_data_alignment_config(ADC_DATAALIGN_RIGHT);

  // Set ADC to scan mode
  adc_special_function_config(ADC_SCAN_MODE, ENABLE);

  // Enable trigger and set trigger source event of ADC
  adc_external_trigger_config(ADC_INSERTED_CHANNEL, ENABLE);
  adc_external_trigger_source_config(ADC_INSERTED_CHANNEL, ADC_EXTTRIG_INSERTED_T0_CH3);

  #ifdef SIMPLEFOC_GD32_ADC_INTERRUPT
  /* clear the ADC Interrupt flag */
  adc_interrupt_flag_clear(ADC_INT_EOIC);

  // Enable interrupt for Inserted Group
  adc_interrupt_enable(ADC_INT_EOIC);

  /* Enable ISR */
  nvic_irq_enable(ADC_CMP_IRQn, 0, 0);
  #endif

  // Enable ADC (must be before calibration)
  adc_enable();

  delay(1U);

  // Calibrate ADC values
  adc_calibration_enable();

  return cs_params;
}

// since SimpleFOC 2.3.4 it returns the current sense params (or SIMPLEFOC_CURRENT_SENSE_INIT_FAILED)
void* _driverSyncLowSide(void* _driver_params, void* _cs_params){
  GD32DriverParams* driver_params = (GD32DriverParams*)_driver_params;

  // the ADC is triggered by TIMER0 CH3, which only the GD32 6PWM driver configures
  // (params: 0 if driver.init() was not called, SIMPLEFOC_DRIVER_INIT_FAILED if it failed)
  if (driver_params == nullptr || driver_params == SIMPLEFOC_DRIVER_INIT_FAILED
      || driver_params->interface_type != _HARDWARE_6PWM) return SIMPLEFOC_CURRENT_SENSE_INIT_FAILED;

  // Start T0CH3 for current sampling
  timer_channel_output_state_config(TIMER_BLDC,TIMER_BLDC_CHANNEL_S,TIMER_CCX_ENABLE);

  return _cs_params;
}

// function reading an ADC value and returning the read voltage
float _readADCVoltageLowSide(const int pin, const void* _cs_params){
  const GD32CurrentSenseParams* cs_params = (const GD32CurrentSenseParams*)_cs_params;
  for(int i=0; i < 3; i++){
    if( pin == cs_params->pins[i]) // found in the buffer
      #ifdef SIMPLEFOC_GD32_ADC_INTERRUPT
      return adc_val[i] * cs_params->adc_voltage_conv;
      #else
      return adc_inserted_data_read(i) * cs_params->adc_voltage_conv;
      #endif
  }
  return 0;
}

// Regular conversion on the current sense ADC (12 bit), for any ADC pin, several pins can be read in turn.
// The ADC runs in the continuous mode set by the core (first analogRead or get_adc_value): it converts the regular
// channel again and again, and the injected group (phase currents) interrupts it at each PWM trigger.
// The end of conversion flag (EOC) is therefore almost always set, and it is also set at the end of the injected group:
// it does not tell when the new channel is converted. After the channel change we wait for 4 ends of conversion:
// the first may be the old channel, two may be injected groups (center-aligned timer: the trigger channel matches on
// the up and the down count, ~2.5 us apart near the top), so the last regular result is the new channel. A PWM
// period is much longer than these ~8 us. The injected group does not write the regular data. EOIC (end of the
// injected group) cannot tell the ends apart: with SIMPLEFOC_GD32_ADC_INTERRUPT the interrupt clears it at once.
// The read used to go through the core's get_adc_value (~0.6 ms on the GD32F130, mostly code in the slow flash); it
// gave the right channel only because it was slow (Candas1/simplefoc-work#22).
// Do not read a phase current pin with this function: the sample time is set per channel, the injected
// conversion of that phase would go from 1.5 to 7.5 cycles.
float _readRegularADCVoltage(const int pin){
  PinName pin_name = DIGITAL_TO_PINNAME(pin);
  uint32_t function = pinmap_function(pin_name, PinMap_ADC);
  if (function == (uint32_t)NC) {
    SIMPLEFOC_DEBUG("GD32-CS: ERR: not an ADC pin: ", pin);
    return -1.0f;
  }
  uint8_t channel = GD_PIN_CHANNEL_GET(function);  // on the GD32F1x0, ADC_CHANNEL_n is n

  // analog mode only if the pin is not in it yet (pinMode is slow)
  uint32_t port = gpio_port[GD_PORT_GET(pin_name)];
  uint32_t pin_mode = (GPIO_CTL(port) >> (2 * GD_PIN_GET(pin_name))) & 0x3U;
  if (pin_mode != GPIO_MODE_ANALOG) pinMode(pin, INPUT_ANALOG);

  // the current sense init turns the ADC on but does not set the continuous mode: let the core finish the init once
  // (configuration and calibration, at its first read only), and set the continuous mode if something cleared it
  if (!(ADC_CTL1 & ADC_CTL1_CTN)) {
    get_adc_value(pin_name);
    if (!(ADC_CTL1 & ADC_CTL1_CTN)) adc_special_function_config(ADC_CONTINUOUS_MODE, ENABLE);
  }

  // 7.5 cycles as the core (1.5 cycles froze the first injected channel with the continuous mode on)
  adc_regular_channel_config(0U, channel, ADC_SAMPLETIME_7POINT5);
  adc_software_trigger_enable(ADC_REGULAR_CHANNEL);
  adc_flag_clear(ADC_FLAG_EOC);
  const int ends_to_wait = 4;  // old channel + up to 2 injected groups + the new channel
  int ends = 0;
  // bound: ~8 us are enough; 1000 turns are far more (each turn reads a register, 10 to 150 cycles)
  for (int turns = 0; ends < ends_to_wait; turns++) {
    if (turns > 1000) {
      static bool reported = false;  // report once (for any pin), not at each call
      if (!reported) SIMPLEFOC_DEBUG("GD32-CS: ERR: regular ADC conversion timeout, first on pin ", pin);
      reported = true;
      return -1.0f;
    }
    if (adc_flag_get(ADC_FLAG_EOC)) {
      adc_flag_clear(ADC_FLAG_EOC);
      ends++;
    }
  }
  return adc_regular_data_read() * _ADC_VOLTAGE_F1 / _ADC_RESOLUTION_F1;
}

#ifdef SIMPLEFOC_GD32_ADC_INTERRUPT
extern "C" {
  void ADC_CMP_IRQHandler(void){
    if ( adc_interrupt_flag_get(ADC_INT_EOIC) != RESET){
      adc_interrupt_flag_clear(ADC_INT_EOIC);
      adc_val[0] = adc_inserted_data_read(ADC_INSERTED_CHANNEL_0);
      adc_val[1] = adc_inserted_data_read(ADC_INSERTED_CHANNEL_1);
      adc_val[2] = adc_inserted_data_read(ADC_INSERTED_CHANNEL_2);
    }
  }
}
#endif

#endif