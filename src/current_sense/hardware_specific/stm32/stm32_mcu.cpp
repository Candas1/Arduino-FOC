
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

#include "stm32_mcu.h"
#include "../../../communication/SimpleFOCDebug.h"

#define _ADC_VOLTAGE 3.3f
#define _ADC_RESOLUTION 4096.0f

// does adc interrupt need a downsample - per adc (5)
bool needs_downsample[ADC_COUNT] = {1};
// downsampling variable - per adc (5)
uint8_t tim_downsample[ADC_COUNT] = {0};

#ifdef SIMPLEFOC_STM32_ADC_INTERRUPT
uint8_t use_adc_interrupt = 1;
#else
uint8_t use_adc_interrupt = 0;
#endif

// function reading an ADC value and returning the read voltage
void* _configureADCInline(const void* driver_params, const int pinA,const int pinB,const int pinC){
  _UNUSED(driver_params);

  if( _isset(pinA) ) pinMode(pinA, INPUT);
  if( _isset(pinB) ) pinMode(pinB, INPUT);
  if( _isset(pinC) ) pinMode(pinC, INPUT);

  Stm32CurrentSenseParams* params = new Stm32CurrentSenseParams {
    .pins = { pinA, pinB, pinC },
    .adc_voltage_conv = (_ADC_VOLTAGE)/(_ADC_RESOLUTION),
    .samples = {NP,NP,NP},
    .inj_trigger = NP,
    .reg_trigger = NP,
  };

  return params;
}

#if defined(STM32F1xx) || defined(STM32F4xx) || defined(STM32G4xx) || defined(STM32L4xx) 
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

  Stm32CurrentSenseParams* cs_params= new Stm32CurrentSenseParams {
    .pins={(int)NOT_SET, (int)NOT_SET, (int)NOT_SET},
    .adc_voltage_conv = (_ADC_VOLTAGE) / (_ADC_RESOLUTION),
    .samples = {NP,NP,NP},
    .inj_trigger = NP,
    .reg_trigger = ADC_SOFTWARE_START,
  };
  _adc_gpio_init(cs_params, pinA,pinB,pinC);

  if(_adc_init(cs_params, (STM32DriverParams*)driver_params) != 0) return SIMPLEFOC_CURRENT_SENSE_INIT_FAILED;
  return cs_params;
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
    cs_params->inj_trigger = _timerToInjectedTRGO(driver_params->timers[tim_num++]);
    if(cs_params->inj_trigger == _TRGO_NOT_AVAILABLE) continue; // timer does not have valid trgo for injected channels

    //cs_params->reg_trigger = _timerToRegularTRGO(driver_params->timers[tim_num++]);
    //if(cs_params->reg_trigger == _TRGO_NOT_AVAILABLE) continue; // timer does not have valid trgo for injected channels

    // this will be the timer with which the ADC will sync
    cs_params->timer_handle = driver_params->timers[tim_num-1];
    // done
    break;
  }
  if( cs_params->timer_handle == NP ){
    // not possible to use these timers for low-side current sense
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot sync any timer to injected channels!");
    #endif
    return -1;
  }

  for(int i=0;i<3;i++){
    if _isset(cs_params->pins[i]){
      cs_params->samples[i] = _add_ADC_sample(cs_params->pins[i],cs_params->inj_trigger,0);
      if (cs_params->samples[i] == -1) return -1;
    }    
  }
  
  #ifdef ARDUINO_B_G431B_ESC1
  if (_add_ADC_sample(A_BEMF1,cs_params->reg_trigger,1) == -1) return -1;
  if (_add_ADC_sample(A_BEMF2,cs_params->reg_trigger,1) == -1) return -1;
  if (_add_ADC_sample(A_BEMF3,cs_params->reg_trigger,1) == -1) return -1;
  if (_add_ADC_sample(A_POTENTIOMETER,cs_params->reg_trigger,1) == -1) return -1;
  if (_add_ADC_sample(A_TEMPERATURE,cs_params->reg_trigger,1) == -1) return -1;
  if (_add_ADC_sample(A_VBUS,cs_params->reg_trigger,1) == -1) return -1;
  #endif

  if (_init_ADCs() == -1) return -1; 

  return 0;
}

void _driverSyncLowSide(void* _driver_params, void* _cs_params){
  STM32DriverParams* driver_params = (STM32DriverParams*)_driver_params;
  Stm32CurrentSenseParams* cs_params = (Stm32CurrentSenseParams*)_cs_params;
 
  // if compatible timer has not been found
  if (cs_params->timer_handle == NULL) return;
  
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
    // remember that this timer has repetition counter - no need to downsample
    needs_downsample[0] = 0;
  }else{
    if(!use_adc_interrupt){
      // If the timer has no repetition counter, it needs to use the interrupt to downsample for low side sensing
      use_adc_interrupt = 1;
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: timer has no repetition counter, ADC interrupt has to be used");
      #endif
    }
  }
  
  // set the trigger output event
  LL_TIM_SetTriggerOutput(cs_params->timer_handle->getHandle()->Instance, LL_TIM_TRGO_UPDATE);
 
  _start_ADCs();

  // restart all the timers of the driver
  _startTimers(driver_params->timers, 6);
}

// function reading an ADC value and returning the read voltage
float _readADCVoltageLowSide(const int pin, const void* cs_params){
  if (!use_adc_interrupt) _read_ADCs(); // Fill the adc buffer now in case no interrup is used
  for(int i=0; i < 3; i++){
    if( pin == ((Stm32CurrentSenseParams*)cs_params)->pins[i]){ // found in the buffer
      return _read_ADC_sample(((Stm32CurrentSenseParams*)cs_params)->samples[i]) * ((Stm32CurrentSenseParams*)cs_params)->adc_voltage_conv;
    }
  } 
  return 0;
}

extern "C" {
  void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *AdcHandle){
    // calculate the instance
    int adc_index = _adcToIndex(AdcHandle);

    // if the timer han't repetition counter - downsample two times
    if( needs_downsample[adc_index] && tim_downsample[adc_index]++ > 0) {
      tim_downsample[adc_index] = 0;
      return;
    }
    
    _start_reg_conversion_ADCs();

    _read_ADCs(); // fill the ADC buffer
  }
}


#endif