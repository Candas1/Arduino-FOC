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