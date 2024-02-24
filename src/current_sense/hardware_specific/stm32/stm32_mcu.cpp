
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

#include "stm32_mcu.h"
#include "../../../communication/SimpleFOCDebug.h"

#define _ADC_VOLTAGE 3.3f
#define _ADC_RESOLUTION 4096.0f
#define ADC_COUNT 5

ADC_HandleTypeDef hadc1;
ADC_InjectionConfTypeDef sConfigInjected1;
#ifdef ADC2
ADC_HandleTypeDef hadc2;
ADC_InjectionConfTypeDef sConfigInjected2;
#endif
#ifdef ADC3
ADC_HandleTypeDef hadc3;
ADC_InjectionConfTypeDef sConfigInjected3;
#endif
#ifdef ADC4
ADC_HandleTypeDef hadc4;
ADC_InjectionConfTypeDef sConfigInjected4;
#endif
#ifdef ADC5
ADC_HandleTypeDef hadc5;
ADC_InjectionConfTypeDef sConfigInjected5;
#endif

#ifdef ARDUINO_B_G431B_ESC1
OPAMP_HandleTypeDef hopamp1;
OPAMP_HandleTypeDef hopamp2;
OPAMP_HandleTypeDef hopamp3;
#endif

// array of values of 4 injected channels per adc instance (5)
uint32_t adc_val[ADC_COUNT][4]={0};
ADC_HandleTypeDef* adc_handles[ADC_COUNT] = {NP};
int adc_channel_count[ADC_COUNT] = {0};
int adc_channel_rank[ADC_COUNT] = {0};
int index_longest_adc = 0;

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
    .adc_handle = {NP,NP,NP},
    .adc_rank = {NP,NP,NP},
    .adc_index = {NP,NP,NP},
    .trigger_flag = NP,
  };

  return params;
}

// function reading an ADC value and returning the read voltage
__attribute__((weak))  float _readADCVoltageInline(const int pinA, const void* cs_params){
  uint32_t raw_adc = analogRead(pinA);
  return raw_adc * ((Stm32CurrentSenseParams*)cs_params)->adc_voltage_conv;
}

void* _configureADCLowSide(const void* driver_params, const int pinA, const int pinB, const int pinC){

  Stm32CurrentSenseParams* cs_params= new Stm32CurrentSenseParams {
    .pins={(int)NOT_SET, (int)NOT_SET, (int)NOT_SET},
    .adc_voltage_conv = (_ADC_VOLTAGE) / (_ADC_RESOLUTION)
  };
  _adc_gpio_init(cs_params, pinA,pinB,pinC);

  #ifdef ARDUINO_B_G431B_ESC1
  // Initialize Opamps
  _configureOPAMP(&hopamp1,OPAMP1);
	_configureOPAMP(&hopamp2,OPAMP2);
	_configureOPAMP(&hopamp3,OPAMP3);
  #endif

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

#ifdef ARDUINO_B_G431B_ESC1
void _configureOPAMP(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *OPAMPx_Def){
  // could this be replaced with LL_OPAMP calls??
  hopamp->Instance = OPAMPx_Def;
  hopamp->Init.PowerMode = OPAMP_POWERMODE_HIGHSPEED;
  hopamp->Init.Mode = OPAMP_PGA_MODE;
  hopamp->Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_IO0;
  hopamp->Init.InternalOutput = DISABLE;
  hopamp->Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
  hopamp->Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0_BIAS;
  hopamp->Init.PgaGain = OPAMP_PGA_GAIN_16_OR_MINUS_15;
  hopamp->Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
  if (HAL_OPAMP_Init(hopamp) != HAL_OK)
  {
    SIMPLEFOC_DEBUG("HAL_OPAMP_Init failed!");
  }
  HAL_OPAMP_Start(hopamp);
}
#endif

int _adc_init(Stm32CurrentSenseParams* cs_params, const STM32DriverParams* driver_params)
{
  ADC_TypeDef *Instance = {};
  int status;

  // automating TRGO flag finding - hardware specific
  uint8_t tim_num = 0;
  while(driver_params->timers[tim_num] != NP && tim_num < 6){
    cs_params->trigger_flag = _timerToInjectedTRGO(driver_params->timers[tim_num++]);
    if(cs_params->trigger_flag == _TRGO_NOT_AVAILABLE) continue; // timer does not have valid trgo for injected channels

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

  // One round for initializing the adc and counting the number of channels
  for(int i=0;i<3;i++){
    if _isset(cs_params->pins[i]){
      Instance = (ADC_TypeDef*)pinmap_peripheral(analogInputToPinName(cs_params->pins[i]), PinMap_ADC);
      status = _adc_init(cs_params,Instance,_adc_get_handle(Instance));
      if (status!= 0) return -1;
    }    
  }

  // One round for configuring the channels with the total InjectedNbrOfConversion
  for(int i=0;i<3;i++){
    if _isset(cs_params->pins[i]){
      Instance = (ADC_TypeDef*)pinmap_peripheral(analogInputToPinName(cs_params->pins[i]), PinMap_ADC);
      status = _adc_channel_config(cs_params,Instance,_adc_get_handle(Instance),_adc_get_config(Instance),i);
      if (status!= 0) return -1;
    }    
  } 

  return 0;
}

 ADC_HandleTypeDef *_adc_get_handle(ADC_TypeDef* Instance){
  if (Instance == ADC1) return &hadc1;
  #ifdef ADC2
  else if (Instance == ADC2) return &hadc2; 
  #endif
  #ifdef ADC3
  else if (Instance == ADC3) return &hadc3;
  #endif
  #ifdef ADC4
  else if (Instance == ADC4) return &hadc4;
  #endif
  #ifdef ADC5
  else if (Instance == ADC5) return &hadc5;
  #endif
  else return nullptr;
 }

 ADC_InjectionConfTypeDef *_adc_get_config(ADC_TypeDef* Instance){
  if (Instance == ADC1) return &sConfigInjected1;
  #ifdef ADC2
  else if (Instance == ADC2) return &sConfigInjected2; 
  #endif
  #ifdef ADC3
  else if (Instance == ADC3) return &sConfigInjected3;
  #endif
  #ifdef ADC4
  else if (Instance == ADC4) return &sConfigInjected4;
  #endif
  #ifdef ADC5
  else if (Instance == ADC5) return &sConfigInjected5;
  #endif
  else return nullptr;
 }

int _adc_init(Stm32CurrentSenseParams* cs_params, ADC_TypeDef* Instance, ADC_HandleTypeDef* hadc)
{

  if (hadc->Instance == 0){
    // This is the first channel configuration of this ADC 
    hadc->Instance = Instance;

    #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: Using ADC: ", _adcToIndex(hadc)+1);
    #endif

    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    #ifdef ADC_RESOLUTION_12B
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    #endif
    hadc->Init.ScanConvMode = ENABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    
    #if !defined(STM32F1xx) && !defined(STM32F2xx) && !defined(STM32F3xx) && \
        !defined(STM32F4xx) && !defined(STM32F7xx) && !defined(STM32G4xx) && \
        !defined(STM32H5xx) && !defined(STM32H7xx) && !defined(STM32L4xx) &&  \
        !defined(STM32L5xx) && !defined(STM32MP1xx) && !defined(STM32WBxx) || \
        defined(ADC_SUPPORT_2_5_MSPS)
    hadc->Init.LowPowerAutoPowerOff  = DISABLE;                       /* ADC automatically powers-off after a conversion and automatically wakes-up when a new conversion is triggered */
    #endif

    hadc->Init.DiscontinuousConvMode = DISABLE;
    #if !defined(STM32F1xx) && !defined(ADC1_V2_5)
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    #endif
    hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START; // for now
    #ifdef ADC_DATAALIGN_RIGHT
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    #endif
    #if !defined(STM32F0xx) && !defined(STM32L0xx)
    hadc->Init.NbrOfConversion = 1;
    #endif
    hadc->Init.DMAContinuousRequests = DISABLE;
    #ifdef ADC_EOC_SINGLE_CONV
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    #endif
    #ifdef ADC_OVR_DATA_PRESERVED
    hadc->Init.Overrun = ADC_OVR_DATA_PRESERVED;
    #endif
    if ( HAL_ADC_Init(hadc) != HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init ADC!");
    #endif
      return -1;
    }
  }

  adc_handles[_adcToIndex(hadc)] = hadc; // Store in list of adc handles to be able to loop through later
  adc_channel_count[_adcToIndex(hadc)]++; // Increment total channel count for this ADC
  index_longest_adc = max(index_longest_adc,adc_channel_count[_adcToIndex(hadc)]); // To find the adc that will finish sampling latest
  
  return 0;
}

int _adc_channel_config(Stm32CurrentSenseParams* cs_params, ADC_TypeDef* Instance, ADC_HandleTypeDef* hadc, ADC_InjectionConfTypeDef* sConfigInjected, int pin_index)
{
  sConfigInjected->ExternalTrigInjecConv = cs_params->trigger_flag;
  #ifdef ADC_EXTERNALTRIGINJECCONV_EDGE_RISING
  sConfigInjected->ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
  #endif
  #ifdef ADC_EXTERNALTRIGINJECCONVEDGE_RISING
  sConfigInjected->ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;  
  #endif
  
  #if defined(STM32F1xx)
  sConfigInjected->InjectedSamplingTime = ADC_SAMPLETIME_1CYCLE_5
  #endif
  #if defined(STM32F4xx)
  sConfigInjected->InjectedSamplingTime = ADC_SAMPLETIME_3CYCLES;
  #endif
  #if defined(STM32G4xx) || defined(STM32L4xx)
  sConfigInjected->InjectedSamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  #endif
  
  sConfigInjected->AutoInjectedConv = DISABLE;
  #if defined(ADC_DIFFERENTIAL_ENDED) && !defined(ADC1_V2_5)
  sConfigInjected->InjectedSingleDiff = ADC_SINGLE_ENDED;
  sConfigInjected->InjectedOffsetNumber = ADC_OFFSET_NONE;
  #endif
  sConfigInjected->InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected->InjectedOffset = 0;
  //sConfigInjected->InjecOversamplingMode = DISABLE;
  //sConfigInjected->QueueInjectedContext = DISABLE;
  

  int adc_index = _adcToIndex(hadc);

  sConfigInjected->InjectedNbrOfConversion = adc_channel_count[adc_index];
  sConfigInjected->InjectedRank = _getInjADCRank(adc_channel_rank[adc_index] + 1);
  sConfigInjected->InjectedChannel = _getADCChannel(analogInputToPinName(cs_params->pins[pin_index]));
  if (HAL_ADCEx_InjectedConfigChannel(hadc, sConfigInjected) != HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init injected channel: ", (int)_getADCChannel(analogInputToPinName(cs_params->pins[pin_index])));
    #endif
    return -1;
  }

  cs_params->adc_handle[pin_index] = hadc; // Use to read the injected adc register
  cs_params->adc_rank[pin_index] = sConfigInjected->InjectedRank; // Use to read the injected adc register
  cs_params->adc_index[pin_index] = adc_channel_rank[adc_index]; // Index to be used when reading from adc_val array
  adc_channel_rank[adc_index]++;

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
    // remember that this timer has repetition counter - no need to downasmple
    needs_downsample[_adcToIndex(cs_params->adc_handle[0])] = 0;
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


// Calibrates the ADC if initialized and not already started
int _calibrate_ADC(ADC_HandleTypeDef* hadc){
  if (hadc->Instance == 0) return 0; // ADC not initialized
  if (LL_ADC_IsEnabled(hadc->Instance)) return 0; // ADC already started
  if (adc_channel_count[_adcToIndex(hadc)] == 0) return 0; // This ADC has no channel to sample

  // Start the adc calibration
#if defined(ADC_CR_ADCAL) || defined(ADC_CR2_RSTCAL)
  /*##-2.1- Calibrate ADC then Start the conversion process ####################*/
  #if defined(ADC_CALIB_OFFSET)
  if (HAL_ADCEx_Calibration_Start(hadc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
  #elif defined(ADC_SINGLE_ENDED) && !defined(ADC1_V2_5)
  if (HAL_ADCEx_Calibration_Start(hadc, ADC_SINGLE_ENDED) !=  HAL_OK)
  #else
  if (HAL_ADCEx_Calibration_Start(hadc) !=  HAL_OK)
  #endif
  {
    /* ADC Calibration Error */
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: can't calibrate ADC :",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }

  return 0;
#endif
}


// Starts the ADC if initialized and not already started
int _start_ADC(ADC_HandleTypeDef* hadc){

  if (hadc->Instance == 0) return 0; // ADC not initialized
  if (LL_ADC_IsEnabled(hadc->Instance)) return 0; // ADC already started
  if (adc_channel_count[_adcToIndex(hadc)] == 0) return 0; // This ADC has no channel to sample

  if (HAL_ADCEx_InjectedStart(hadc) !=  HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: can't start inj ADC :",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}

// Starts the ADC with interrupt if initialized and not already started
int _start_ADC_IT(ADC_HandleTypeDef* hadc){

  if (hadc->Instance == 0) return 0; // ADC not initialized
  if (LL_ADC_IsEnabled(hadc->Instance)) return 0; // ADC already started
  if (adc_channel_count[_adcToIndex(hadc)] == 0) return 0; // This ADC has no channel to sample
      
  // enable interrupt
  #if defined(STM32F4xx)
  HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC_IRQn);
  #endif

  #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
  if(hadc->Instance == ADC1 || hadc->Instance == ADC2) {
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
  }
  
  #ifdef ADC2
    else if (hadc->Instance == ADC2) {
      HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
    }
  #endif
  #ifdef ADC3
    else if (hadc->Instance == ADC3) {
      HAL_NVIC_SetPriority(ADC3_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(ADC3_IRQn);
    } 
  #endif
  #ifdef ADC4
    else if (hadc->Instance == ADC4) {
      HAL_NVIC_SetPriority(ADC4_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(ADC4_IRQn);
    } 
  #endif
  #ifdef ADC5
    else if (hadc->Instance == ADC5) {
      HAL_NVIC_SetPriority(ADC5_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(ADC5_IRQn);
    } 
  #endif
  #endif

  if (HAL_ADCEx_InjectedStart_IT(hadc) !=  HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: can't start inj ADC with IT:",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}

// Calibrated and starts all the ADCs that have been initialized
int _start_ADCs(void){
  int status = 0;

  for (int i = 0; i < ADC_COUNT; i++){
    if (adc_handles[i] != NP){
      status = _calibrate_ADC(adc_handles[i]);
      if (status!=0) return status;

      if (use_adc_interrupt && (adc_handles[i])->Instance == ADC1) status = _start_ADC_IT(adc_handles[i]);
      else status = _start_ADC(adc_handles[i]);
      if (status!=0) return status;
    }
  }

  return 0;
}

// Writes Injected ADC values to the adc buffer
void _read_ADC(ADC_HandleTypeDef* hadc){

  if (hadc->Instance == 0) return; // skip if ADC not initialized
  if (!LL_ADC_IsEnabled(hadc->Instance)) return; // skip if ADC not started

  int adc_index = _adcToIndex(hadc);
  int channel_count = adc_channel_count[_adcToIndex(hadc)];
  for(int i=0;i<channel_count;i++){
    adc_val[adc_index][i] = HAL_ADCEx_InjectedGetValue(hadc,_getInjADCRank(i+1));
  } 
}

// Writes Injected ADC values to the adc buffer for all ADCs that have been initialized
void _read_ADCs(){
  for (int i = 0; i < ADC_COUNT; i++){
    if (adc_handles[i] != NP){
      _read_ADC(adc_handles[i]);
    }
  }
}

// function reading an ADC value and returning the read voltage
float _readADCVoltageLowSide(const int pin, const void* cs_params){
  if (!use_adc_interrupt) _read_ADCs(); // Fill the adc buffer now in case no interrup is used
  for(int i=0; i < 3; i++){
    if( pin == ((Stm32CurrentSenseParams*)cs_params)->pins[i]){ // found in the buffer
      int index = ((Stm32CurrentSenseParams*)cs_params)->adc_index[i];
      int adc_index = _adcToIndex(((Stm32CurrentSenseParams*)cs_params)->adc_handle[i]); 
      return adc_val[adc_index][index] * ((Stm32CurrentSenseParams*)cs_params)->adc_voltage_conv;
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
    
    _read_ADCs(); // fill the ADC buffer
  }
}


extern "C" {
  #if defined(STM32F4xx)
  void ADC_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(&hadc1);
  }
  #endif

  #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
  void ADC1_2_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(&hadc1);
  }
  #endif

  #if defined(STM32G4xx) || defined(STM32L4xx)
  #ifdef ADC3
  void ADC3_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(&hadc);
  }
  #endif

  #ifdef ADC4
  void ADC4_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(&hadc);
  }
  #endif

  #ifdef ADC5
  void ADC5_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(&hadc);
  }
  #endif
  #endif
}

#endif