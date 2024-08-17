
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

#include "stm32_mcu.h"

#define _ADC_VOLTAGE 3.3f
#define _ADC_RESOLUTION 4096.0f

#ifdef SIMPLEFOC_STM32_ADC_INTERRUPT
uint8_t stm32_use_adc_interrupt = 1;
#else
uint8_t stm32_use_adc_interrupt = 0;
#endif

// function reading an ADC value and returning the read voltage
void* _configureADCInline(const void* driver_params, const int pinA,const int pinB,const int pinC){
  _UNUSED(driver_params);

  if( _isset(pinA) ) pinMode(pinA, INPUT);
  if( _isset(pinB) ) pinMode(pinB, INPUT);
  if( _isset(pinC) ) pinMode(pinC, INPUT);

  Stm32CurrentSenseParams* csparams = new Stm32CurrentSenseParams {
    .pins = { pinA, pinB, pinC },
    .adc_voltage_conv = (_ADC_VOLTAGE)/(_ADC_RESOLUTION),
    .samples = {NP,NP,NP},
    .inj_trigger = NP,
    .reg_trigger = NP,
    .type = INLINE,
    .use_adc_interrupt = NP,
  };

  #if defined(STM_CURRENT_SENSE_SUPPORTED)
  // Those pins will be sampled continuously
  for(int i=0;i<3;i++){
    if _isset(csparams->pins[i]){
      csparams->samples[i] = _add_ADC_sample(csparams->pins[i],ADC_SOFTWARE_START,REGULAR_ADC);
    }    
  }

  _init_ADCs();
  _start_ADCs(csparams);
  #endif

  return csparams;
}

#if defined(STM_CURRENT_SENSE_SUPPORTED)
// function reading an ADC value and returning the read voltage
float _readADCVoltageInline(const int pinA, const void* cs_params){
  uint32_t raw_adc = _read_ADC_pin(pinA);
  return raw_adc * ((Stm32CurrentSenseParams*)cs_params)->adc_voltage_conv;  
}
#else
__attribute__((weak))  float _readADCVoltageInline(const int pinA, const void* cs_params){
  uint32_t raw_adc = analogRead(pinA);
  return raw_adc * ((Stm32CurrentSenseParams*)cs_params)->adc_voltage_conv;
}
#endif

void* _configureADCLowSide(const void* driver_params, const int pinA, const int pinB, const int pinC){

  Stm32CurrentSenseParams* csparams= new Stm32CurrentSenseParams {
    .pins={(int)NOT_SET, (int)NOT_SET, (int)NOT_SET},
    .adc_voltage_conv = (_ADC_VOLTAGE) / (_ADC_RESOLUTION),
    .samples = {NP,NP,NP},
    .inj_trigger = NP,
    .reg_trigger = NP,
    .type = LOWSIDE,
    .use_adc_interrupt = NP,
  };

  // Fail if this familly is not supported
  #ifndef STM_CURRENT_SENSE_SUPPORTED
  return SIMPLEFOC_CURRENT_SENSE_INIT_FAILED;
  #endif

  _adc_gpio_init(csparams, pinA,pinB,pinC);

  if(_adc_init(csparams, (STM32DriverParams*)driver_params) != 0) return SIMPLEFOC_CURRENT_SENSE_INIT_FAILED;
  return csparams;
}

void _adc_gpio_init(Stm32CurrentSenseParams* cs_params, const int pinA, const int pinB, const int pinC)
{
  uint8_t cnt = 0;
  if(_isset(pinA)){
    pinmap_pinout(analogInputToPinName(pinA), PinMap_ADC);
    cs_params->pins[cnt++] = pinA;
  }
  if(_isset(pinB)){
    pinmap_pinout(analogInputToPinName(pinB), PinMap_ADC);
    cs_params->pins[cnt++] = pinB;
  }
  if(_isset(pinC)){ 
    pinmap_pinout(analogInputToPinName(pinC), PinMap_ADC);
    cs_params->pins[cnt] = pinC;
  }
}

int _adc_init(Stm32CurrentSenseParams* cs_params, const STM32DriverParams* driver_params)
{
  ADC_TypeDef *Instance = {};
  int status;

  // automating TRGO flag finding - hardware specific
  uint8_t tim_num = 0;
  while(driver_params->timers[tim_num] != NP && tim_num < 6){
    #if defined(ADC_INJECTED_SOFTWARE_START)
    // Injected ADC is available
    cs_params->inj_trigger = _timerToInjectedTRGO(driver_params->timers[tim_num++]);
    if(cs_params->inj_trigger == _TRGO_NOT_AVAILABLE) continue; // timer does not have valid trgo for injected channels
    #else
    // Injected ADC is not available, use regular ADC
    cs_params->reg_trigger = _timerToRegularTRGO(driver_params->timers[tim_num++]);
    if(cs_params->reg_trigger == _TRGO_NOT_AVAILABLE) continue; // timer does not have valid trgo for injected channels
    #endif

    // this will be the timer with which the ADC will sync
    cs_params->timer_handle = driver_params->timers[tim_num-1];
    // done
    break;
  }

  if( cs_params->timer_handle == NP ){
    // not possible to use these timers for low-side current sense
    #ifdef SIMPLEFOC_STM32_DEBUG
    #if defined(ADC_INJECTED_SOFTWARE_START)
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot sync any timer to injected channels!");
    #else
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot sync any timer to regular channels!");
    #endif
    #endif
    return -1;
  }

  cs_params->use_adc_interrupt = stm32_use_adc_interrupt;
  if (!IS_TIM_REPETITION_COUNTER_INSTANCE(cs_params->timer_handle->getHandle()->Instance) && !cs_params->use_adc_interrupt){
    // With low side current sensing, if the timer has no repetition counter, it needs to use the interrupt to sample at V0 only 
    cs_params->use_adc_interrupt = 1;
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: timer has no repetition counter, ADC interrupt has to be used");
    #endif
  }

  for(int i=0;i<3;i++){
    if _isset(cs_params->pins[i]){
      #if defined(ADC_INJECTED_SOFTWARE_START)
      cs_params->samples[i] = _add_ADC_sample(cs_params->pins[i],cs_params->inj_trigger,INJECTED_ADC,cs_params);
      #else
      cs_params->samples[i] = _add_ADC_sample(cs_params->pins[i],cs_params->reg_trigger,REGULAR_ADC,cs_params);
      #endif
      if (cs_params->samples[i] == -1) return -1;
    }    
  }

  #ifdef ARDUINO_B_G431B_ESC1
  // Add other channels to sample on this specific board
  if (_add_ADC_sample(A_BEMF1,cs_params->inj_trigger,INJECTED_ADC) == -1) return -1;
  if (_add_ADC_sample(A_BEMF2,cs_params->inj_trigger,INJECTED_ADC) == -1) return -1;
  if (_add_ADC_sample(A_BEMF3,cs_params->inj_trigger,INJECTED_ADC) == -1) return -1;
  if (_add_ADC_sample(A_POTENTIOMETER,ADC_SOFTWARE_START,REGULAR_ADC) == -1) return -1;
  if (_add_ADC_sample(A_TEMPERATURE,ADC_SOFTWARE_START,REGULAR_ADC) == -1) return -1;
  if (_add_ADC_sample(A_VBUS,ADC_SOFTWARE_START,REGULAR_ADC) == -1) return -1;
  
  // Initialize Opamps
  if (_init_OPAMPs() == -1) return -1;
  #endif

  if (_init_ADCs() == -1) return -1; 

  return 0;
}

void* _driverSyncLowSide(void* _driver_params, void* _cs_params){
  STM32DriverParams* driver_params = (STM32DriverParams*)_driver_params;
  Stm32CurrentSenseParams* cs_params = (Stm32CurrentSenseParams*)_cs_params;
 
  // if compatible timer has not been found
  if (cs_params->timer_handle == NULL) return SIMPLEFOC_CURRENT_SENSE_INIT_FAILED;
  
  // stop all the timers for the driver
  _stopTimers(driver_params->timers, 6);

  // if timer has repetition counter - it will downsample using it
  // and it does not need the software downsample
  if( IS_TIM_REPETITION_COUNTER_INSTANCE(cs_params->timer_handle->getHandle()->Instance) ){
    // adjust the initial timer state such that the trigger 
    //   - for DMA transfer aligns with the pwm peaks instead of throughs.
    //   - for interrupt based ADC transfer 
    //   - only necessary for the timers that have repetition counters
    cs_params->timer_handle->getHandle()->Instance->CR1 |= TIM_CR1_DIR;
    cs_params->timer_handle->getHandle()->Instance->CNT =  cs_params->timer_handle->getHandle()->Instance->ARR;
  }
  
  // set the trigger output event
  LL_TIM_SetTriggerOutput(cs_params->timer_handle->getHandle()->Instance, LL_TIM_TRGO_UPDATE);
 
  _start_ADCs(cs_params);

  // restart all the timers of the driver
  _startTimers(driver_params->timers, 6);

  // return the cs parameters 
  // successfully initialized
  // TODO verify if success in future
  return _cs_params;

}

// function reading an ADC value and returning the read voltage
float _readADCVoltageLowSide(const int pin, const void* cs_params){
  for(int i=0; i < 3; i++){
    if( pin == ((Stm32CurrentSenseParams*)cs_params)->pins[i]){ // found in the buffer
      return _read_ADC_sample(((Stm32CurrentSenseParams*)cs_params)->samples[i]) * ((Stm32CurrentSenseParams*)cs_params)->adc_voltage_conv;
    }
  } 
  return 0;
}

extern "C" {
  // Only if injected ADC available
  #ifdef ADC_INJECTED_SOFTWARE_START
  void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *AdcHandle){   
    _buffer_ADCs(); // fill the ADC buffer
  }
  #endif
}

#endif