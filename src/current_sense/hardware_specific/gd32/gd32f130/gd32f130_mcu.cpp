#include "../../../hardware_api.h"

#if defined(GD32F1x0) 
#include "../../../../common/foc_utils.h"
#include "../../../../common/time_utils.h"
#include "../../../../drivers/hardware_api.h"
#include "../../../../drivers/hardware_specific/gd32/gd32_mcu.h"
#include "../../../hardware_api.h"
#include "../gd32_mcu.h"
#include "gd32/PinNames.h"
#include "Arduino.h"

#define _ADC_VOLTAGE_F1 3.3f
#define _ADC_RESOLUTION_F1 4096.0f
#define SAMPLING_TIME ADC_SAMPLETIME_1POINT5

// array of values of 4 injected channels per adc instance (3)
uint32_t adc_val[4]={0};

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
  // No continuous mode: the first analogRead of the core enables it, then any later regular conversion
  // (analogRead, _readRegularADCVoltage) runs continuously and the first injected channel stops being updated
  // (phase A current frozen, measured on the GD32F130 split board)
  adc_special_function_config(ADC_CONTINUOUS_MODE, DISABLE);

  // Disable the temperature sensor, Vrefint and vbat channel
	//adc_tempsensor_vrefint_disable();
	//adc_vbat_disable();
	
	// ADC analog watchdog disable
	//adc_watchdog_disable();

	// Enable trigger and set trigger source event of ADC
	adc_external_trigger_config(ADC_INSERTED_CHANNEL, ENABLE);
	adc_external_trigger_source_config(ADC_INSERTED_CHANNEL, ADC_EXTTRIG_INSERTED_T0_CH3);
	

  #ifdef ADC_USE_INTERRUPT
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

  // Start T0CH3 for current sampling
  timer_channel_output_state_config(TIMER_BLDC,TIMER_BLDC_CHANNEL_S,ENABLE);

  return _cs_params;
}

// function reading an ADC value and returning the read voltage
float _readADCVoltageLowSide(const int pin, const void* cs_params){
  for(int i=0; i < 3; i++){
    if( pin == ((GD32CurrentSenseParams*)cs_params)->pins[i]) // found in the buffer
      #ifdef ADC_USE_INTERRUPT
      return adc_val[i] * ((GD32CurrentSenseParams*)cs_params)->adc_voltage_conv;
      #else
      return adc_inserted_data_read(i) * ((GD32CurrentSenseParams*)cs_params)->adc_voltage_conv;
      #endif
  } 
  return 0;
}


// last pin read by _readRegularADCVoltage and its ADC channel (the pin is configured once)
int last_regular_pin = NOT_SET;
uint8_t last_regular_channel = 0;

float _readRegularADCVoltage(const int pin){
  if (pin != last_regular_pin) {
    PinName pin_name = DIGITAL_TO_PINNAME(pin);
    if (!pin_in_pinmap(pin_name, PinMap_ADC)) {
      SIMPLEFOC_DEBUG("GD32-CS: ERR: not an ADC pin: ", pin);
      return -1.0f;
    }
    // analog mode with pinMode: does not go through the pinmap AF of the core (see GD_PIN_AF_GET)
    pinMode(pin, INPUT_ANALOG);
    last_regular_channel = get_adc_channel(pin_name);
    last_regular_pin = pin;
  }

  // ADC not enabled yet by the current sense: same clock as _configureADCLowSide, then calibration
  if (!(ADC_CTL1 & ADC_CTL1_ADCON)) {
    rcu_periph_clock_enable(RCU_ADC);
    rcu_adc_clock_config(RCU_ADCCK_APB2_DIV6);
    adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
    adc_enable();
    delay(1U);
    adc_calibration_enable();
  }

  // one regular channel, single conversion started by software (the injected group keeps its timer trigger).
  // Continuous mode off again: an analogRead called before may have enabled it (see _configureADCLowSide)
  adc_special_function_config(ADC_CONTINUOUS_MODE, DISABLE);
  adc_channel_length_config(ADC_REGULAR_CHANNEL, 1);
  adc_external_trigger_source_config(ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_NONE);
  adc_external_trigger_config(ADC_REGULAR_CHANNEL, ENABLE);
  adc_regular_channel_config(0, last_regular_channel, SAMPLING_TIME);

  adc_flag_clear(ADC_FLAG_EOC);
  adc_software_trigger_enable(ADC_REGULAR_CHANNEL);
  // a regular conversion takes ~1.2 us at 12 MHz, plus the injected ones that interrupt it: 1 ms timeout
  unsigned long start = _micros();
  while (!adc_flag_get(ADC_FLAG_EOC)) {
    if (_micros() - start > 1000) {
      SIMPLEFOC_DEBUG("GD32-CS: ERR: regular conversion timeout");
      return -1.0f;
    }
  }
  adc_flag_clear(ADC_FLAG_EOC);
  return adc_regular_data_read() * _ADC_VOLTAGE_F1 / _ADC_RESOLUTION_F1;
}


#ifdef ADC_USE_INTERRUPT
extern "C" {  
  void ADC_CMP_IRQHandler(void){
    if ( adc_interrupt_flag_get(ADC_INT_EOIC) != RESET){
      //GPIO_BOP(GPIOB) = (uint32_t)GPIO_PIN_7; // Turning output ON for measurements on the oscilloscope
      adc_interrupt_flag_clear(ADC_INT_EOIC);
      adc_val[0] = (1 - PHASE_CURRENT_FILTER) * adc_val[0] + PHASE_CURRENT_FILTER * adc_inserted_data_read(ADC_INSERTED_CHANNEL_0);
      adc_val[1] = (1 - PHASE_CURRENT_FILTER) * adc_val[1] + PHASE_CURRENT_FILTER * adc_inserted_data_read(ADC_INSERTED_CHANNEL_1);
      adc_val[2] = (1 - PHASE_CURRENT_FILTER) * adc_val[2] + PHASE_CURRENT_FILTER * adc_inserted_data_read(ADC_INSERTED_CHANNEL_2);
      //GPIO_BC(GPIOB) = (uint32_t)GPIO_PIN_7; // Turning output OFF for measurements on the oscilloscope
    }
  }
}
#endif

#endif