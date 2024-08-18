
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

#include "stm32_adc.h"

ADC_HandleTypeDef hadc1;
#ifdef ADC2
ADC_HandleTypeDef hadc2;
#endif
#ifdef ADC3
ADC_HandleTypeDef hadc3;
#endif
#ifdef ADC4
ADC_HandleTypeDef hadc4;
#endif
#ifdef ADC5
ADC_HandleTypeDef hadc5;
#endif

Stm32ADCEngine ADCEngine;

ADC_HandleTypeDef *interrupt_adc = NP;

ADC_HandleTypeDef *_get_ADC_handle(ADC_TypeDef* Instance){
  
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

int _add_ADC_sample(uint32_t pin,int32_t trigger,adc_type type,void* _cs_params){
  ADC_TypeDef *Instance;
  Stm32ADCSample sample = {};
  
  #ifndef ADC_INJECTED_SOFTWARE_START
  if (type == INJECTED_ADC){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: inj ADC not available: ", (int) sample.adc_index+1);
    #endif
  }
  #endif
  
  PinName pinname = analogInputToPinName(pin);
  
  // If internal channel, the ADC instance depends on the chip family
  if ((pin & PADC_BASE) && (pin < ANA_START)) {
  #if defined(STM32H7xx) || defined(STM32MP1xx)
  #ifdef ADC3
      Instance = ADC3;
  #else
      Instance = ADC2;
  #endif
  #elif defined(STM32WBAxx)
      Instance = ADC4;
  #else
      Instance = ADC1;
  #if defined(ADC5) && defined(ADC_CHANNEL_TEMPSENSOR_ADC5)
      if (pin == PADC_TEMP_ADC5) {
        Instance = ADC5;
      }
  #endif
  #endif
      sample.channel = get_adc_internal_channel(pinname);
      sample.SamplingTime = ADC_SAMPLINGTIME_INTERNAL; // Long sampling time for internal channels
  } else {
      // This is not an internal channel, Initialize the pin mode
      pinmap_pinout(pinname, PinMap_ADC);
      
      // Identify the adc instance from the pinmap
      Instance = (ADC_TypeDef*)pinmap_peripheral(pinname, PinMap_ADC);
      sample.channel   = _getADCChannel(pinname);
      if (type == INJECTED_ADC){
        sample.SamplingTime = ADC_SAMPLINGTIME_INJ; // Short sampling time for injected adc sampling
      }else{
        sample.SamplingTime = ADC_SAMPLINGTIME; // Longer sampling time for regular adc sampling
      }
  }
  
  
  ADC_HandleTypeDef* hadc = _get_ADC_handle(Instance);
  int adc_index = _adcToIndex(Instance);

  ADCEngine.adc_handles[adc_index] = hadc; // Store in list of adc handles to be able to loop through later
  
  sample.Instance  = Instance;
  sample.type      = type; // 0 = inj, 1 = reg
  sample.handle    = hadc;
  sample.pin       = pin;
  sample.adc_index = adc_index;
  sample.cs_params = _cs_params;

  sample.buffered = 0;
  if (_cs_params != nullptr){
    Stm32CurrentSenseParams* cs_params = (Stm32CurrentSenseParams*)_cs_params;
    if (cs_params->use_adc_interrupt && cs_params->type == LOWSIDE) sample.buffered = 1;
  }
  
  if (type == INJECTED_ADC){
    #ifdef ADC_INJECTED_SOFTWARE_START
    if (ADCEngine.inj_channel_count[adc_index] == MAX_INJ_ADC_CHANNELS){
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: max inj channel reached: ", (int) sample.adc_index+1);
      #endif  
    }

    if (ADCEngine.inj_trigger[adc_index] != trigger){
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: inj ADC has different trigger: ", (int) sample.adc_index+1);
      #endif  
    }

    ADCEngine.inj_trigger[adc_index] = trigger;
    sample.rank = _getInjADCRank(ADCEngine.inj_channel_count[adc_index] + 1);
    sample.index = ADCEngine.inj_channel_count[adc_index];
    ADCEngine.inj_channel_count[adc_index]++; // Increment total injected channel count for this ADC
    ADCEngine.inj_channel_max = max(ADCEngine.inj_channel_max,ADCEngine.inj_channel_count[adc_index]); // Longest number of injected adc channels
    #endif
  }else{
    if (ADCEngine.reg_channel_count[adc_index] == MAX_REG_ADC_CHANNELS){
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: max reg channel reached: ", (int) sample.adc_index+1);
      #endif  
    }

    if (ADCEngine.reg_trigger[adc_index] != trigger){
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: reg ADC has different trigger: ", (int) sample.adc_index+1);
      #endif  
    }

    ADCEngine.reg_trigger[adc_index] = trigger;
    sample.rank = _getRegADCRank(ADCEngine.reg_channel_count[adc_index] + 1);
    sample.index = ADCEngine.reg_channel_count[adc_index];
    ADCEngine.reg_channel_count[adc_index]++; // Increment total regular channel count for this ADC
    ADCEngine.reg_channel_max = max(ADCEngine.reg_channel_max,ADCEngine.reg_channel_count[adc_index]); // Longest number of injected adc channels
  }
  
  ADCEngine.samples[ADCEngine.sample_count] = sample;
  ADCEngine.sample_count++;  

  return ADCEngine.sample_count - 1; // Return index of the sample
}

int _init_ADCs(){
  for (int i=0;i<ADCEngine.sample_count;i++){
    if (_init_ADC(ADCEngine.samples[i]) == -1) return -1;
    if (ADCEngine.samples[i].type == INJECTED_ADC ){
      #ifdef ADC_INJECTED_SOFTWARE_START
      if (_add_inj_ADC_channel_config(ADCEngine.samples[i]) == -1) return -1;
      #endif
    }else{
      if (_add_reg_ADC_channel_config(ADCEngine.samples[i]) == -1) return -1;
    }
  }

  return 0;
}

int _init_ADC(Stm32ADCSample sample)
{

  if (sample.handle->Instance != 0) return 0;

  // This is the first channel configuration of this ADC 
  sample.handle->Instance = sample.Instance;

  #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: Using ADC: ", (int) sample.adc_index+1);
  #endif
  
  #ifdef ADC_CLOCK_DIV
  sample.handle->Init.ClockPrescaler = ADC_CLOCK_DIV;
  #endif

  #ifdef ADC_RESOLUTION_12B
  sample.handle->Init.Resolution = ADC_RESOLUTION_12B;
  #endif

  #if defined(ADC_SCAN_ENABLE)
  sample.handle->Init.ScanConvMode = ADC_SCAN_ENABLE; 
  #else
  sample.handle->Init.ScanConvMode = ENABLE;
  #endif

  sample.handle->Init.ContinuousConvMode = ENABLE;
  sample.handle->Init.DiscontinuousConvMode = DISABLE;
  
  #if defined(STM32F0xx)
  sample.handle->Init.SamplingTimeCommon = sample.SamplingTime;
  #endif

  #if defined(STM32L0xx)
  sample.handle->Init.SamplingTime = sample.SamplingTime;
  #endif
  
  #if defined(STM32C0xx) || defined(STM32G0xx) || defined(STM32U5xx) || \
    defined(STM32WBAxx) || defined(STM32WLxx) || defined(ADC_SUPPORT_2_5_MSPS)
  sample.handle->Init.SamplingTimeCommon1   = sample.SamplingTime;              /* Set sampling time common to a group of channels. */
  sample.handle->Init.SamplingTimeCommon2   = sample.SamplingTime;              /* Set sampling time common to a group of channels, second common setting possible.*/
  #endif
  
  #if !defined(STM32F1xx) && !defined(STM32F2xx) && !defined(STM32F3xx) && \
      !defined(STM32F4xx) && !defined(STM32F7xx) && !defined(STM32G4xx) && \
      !defined(STM32H5xx) && !defined(STM32H7xx) && !defined(STM32L4xx) &&  \
      !defined(STM32L5xx) && !defined(STM32MP1xx) && !defined(STM32WBxx) || \
      defined(ADC_SUPPORT_2_5_MSPS)
  sample.handle->Init.LowPowerAutoPowerOff  = DISABLE;                       /* ADC automatically powers-off after a conversion and automatically wakes-up when a new conversion is triggered */
  #endif

  sample.handle->Init.ExternalTrigConv = ADCEngine.reg_trigger[sample.adc_index];
  #if !defined(STM32F1xx) && !defined(ADC1_V2_5)
  if (sample.handle->Init.ExternalTrigConv == ADC_SOFTWARE_START){
    sample.handle->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  }else{
    sample.handle->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  }
  #endif
  #ifdef ADC_DATAALIGN_RIGHT
  sample.handle->Init.DataAlign = ADC_DATAALIGN_RIGHT;
  #endif
  #if !defined(STM32F0xx) && !defined(STM32L0xx)
  sample.handle->Init.NbrOfConversion = ADCEngine.reg_channel_count[sample.adc_index]; // Minimum 1 for analogread to work
  #endif
  #if !defined(STM32F1xx) && !defined(STM32H7xx) && !defined(STM32MP1xx) && \
      !defined(ADC1_V2_5)
  sample.handle->Init.DMAContinuousRequests = ENABLE;
  #endif
  #ifdef ADC_EOC_SINGLE_CONV
  sample.handle->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  #endif
  #ifdef ADC_OVR_DATA_PRESERVED
  sample.handle->Init.Overrun = ADC_OVR_DATA_PRESERVED;
  #endif

  if (_init_DMA(sample.handle) == -1) return -1;

  if ( HAL_ADC_Init(sample.handle) != HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init ADC!");
    #endif
    return -1;
  }

  return 0;
}

int _add_reg_ADC_channel_config(Stm32ADCSample sample)
{
  ADC_ChannelConfTypeDef  AdcChannelConf = {};
  AdcChannelConf.Channel      = sample.channel;            /* Specifies the channel to configure into ADC */
  AdcChannelConf.Rank         = sample.rank;               /* Specifies the rank in the regular group sequencer */
  
#if !defined(STM32L0xx)
  // On L0, the sampling time is common for all channels and it's set on the ADC
#if !defined(STM32G0xx)
  AdcChannelConf.SamplingTime = sample.SamplingTime;       /* Sampling time value to be set for the selected channel */
#else
  // On G0, you pick from two common sampling times that are set on the ADC
  AdcChannelConf.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;        /* Sampling time value to be set for the selected channel */
#endif
#endif

#if defined(ADC_DIFFERENTIAL_ENDED) && !defined(ADC1_V2_5)
  AdcChannelConf.SingleDiff   = ADC_SINGLE_ENDED;                 /* Single-ended input channel */
  AdcChannelConf.OffsetNumber = ADC_OFFSET_NONE;                  /* No offset subtraction */
#endif
#if !defined(STM32C0xx) && !defined(STM32F0xx) && !defined(STM32F1xx) && \
    !defined(STM32F2xx) && !defined(STM32G0xx) && !defined(STM32L0xx) && \
    !defined(STM32L1xx) && !defined(STM32WBxx) && !defined(STM32WLxx) && \
    !defined(ADC1_V2_5)
  AdcChannelConf.Offset = 0;                                      /* Parameter discarded because offset correction is disabled */
#endif
#if defined (STM32H7xx) || defined(STM32MP1xx)
  AdcChannelConf.OffsetRightShift = DISABLE;                      /* No Right Offset Shift */
  AdcChannelConf.OffsetSignedSaturation = DISABLE;                /* Signed saturation feature is not used */
#endif

  /*##-2- Configure ADC regular channel ######################################*/
  if (HAL_ADC_ConfigChannel(sample.handle, &AdcChannelConf) != HAL_OK) {
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init reg channel: ", (int) AdcChannelConf.Channel);
    #endif
    return -1;
  }

  return 0;
  
}

// Calibrates the ADC if initialized and not already enabled
int _calibrate_ADC(ADC_HandleTypeDef* hadc){
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
    SIMPLEFOC_DEBUG("STM32-CS: ERR: can't calibrate ADC :",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  #endif

  return 0;
}


// Only if Injected ADC is available
#ifdef ADC_INJECTED_SOFTWARE_START
int _add_inj_ADC_channel_config(Stm32ADCSample sample)
{
  ADC_InjectionConfTypeDef sConfigInjected = {};

  #if defined(ADC_EXTERNALTRIGINJECCONVEDGE_RISINGFALLING) 
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;
  #endif
  #if defined(ADC_EXTERNALTRIGINJECCONV_EDGE_RISINGFALLING)
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
  #endif
  
  sConfigInjected.AutoInjectedConv = DISABLE;
  #if defined(ADC_DIFFERENTIAL_ENDED) && !defined(ADC1_V2_5)
  sConfigInjected.InjectedSingleDiff = ADC_SINGLE_ENDED;
  sConfigInjected.InjectedOffsetNumber = ADC_OFFSET_NONE;
  #endif
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.InjectedOffset = 0;
  //sConfigInjected.InjecOversamplingMode = DISABLE;
  //sConfigInjected.QueueInjectedContext = DISABLE;

  int adc_index = _adcToIndex(sample.handle);

  sConfigInjected.ExternalTrigInjecConv   = ADCEngine.inj_trigger[sample.adc_index];
  sConfigInjected.InjectedSamplingTime    = sample.SamplingTime;
  sConfigInjected.InjectedNbrOfConversion = ADCEngine.inj_channel_count[adc_index];
  sConfigInjected.InjectedRank            = sample.rank;
  sConfigInjected.InjectedChannel         = sample.channel;
  if (HAL_ADCEx_InjectedConfigChannel(sample.handle, &sConfigInjected) != HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init inj channel: ", (int) sConfigInjected.InjectedChannel);
    #endif
    return -1;
  }

  return 0;
}

// Starts the injected ADC
int _start_inj_ADC(ADC_HandleTypeDef* hadc){
  #ifdef SIMPLEFOC_STM32_DEBUG
  SIMPLEFOC_DEBUG("STM32-CS: start inj ADC :",_adcToIndex(hadc)+1);
  #endif

  if (HAL_ADCEx_InjectedStart(hadc) !=  HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: can't start inj ADC :",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}

// Starts injected ADC with interrupt
int _start_inj_ADC_IT(ADC_HandleTypeDef* hadc){ 
  #ifdef SIMPLEFOC_STM32_DEBUG
  SIMPLEFOC_DEBUG("STM32-CS: start inj ADC with IT:",_adcToIndex(hadc)+1);
  #endif
  
  _enable_irq(hadc);

  if (HAL_ADCEx_InjectedStart_IT(hadc) !=  HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: can't start inj ADC with IT:",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}
#endif

// Enable IRQ for ADC interrupt
void _enable_irq(ADC_HandleTypeDef* hadc){
  IRQn_Type ADC_IRQ;
  #if defined(STM32F4xx) || defined(STM32F7xx)
  ADC_IRQ = ADC_IRQn;
  #endif

  #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
  if(hadc->Instance == ADC1) ADC_IRQ = ADC1_2_IRQn;
  #ifdef ADC2
    else if (hadc->Instance == ADC2) ADC_IRQ = ADC1_2_IRQn;
  #endif
  #ifdef ADC3
    else if (hadc->Instance == ADC3) ADC_IRQ = ADC3_IRQn;
  #endif
  #ifdef ADC4
    else if (hadc->Instance == ADC4) ADC_IRQ = ADC4_IRQn;
  #endif
  #ifdef ADC5
    else if (hadc->Instance == ADC5) ADC_IRQ = ADC5_IRQn;
  #endif
  #endif

  // enable interrupt
  HAL_NVIC_SetPriority(ADC_IRQ, 0, 0);
  HAL_NVIC_EnableIRQ(ADC_IRQ);
}

// Starts the regular ADC with interrupt
int _start_reg_ADC(ADC_HandleTypeDef* hadc){ 
  #ifdef SIMPLEFOC_STM32_DEBUG
  SIMPLEFOC_DEBUG("STM32-CS: start reg ADC:",_adcToIndex(hadc)+1);
  #endif
  
  if (HAL_ADC_Start(hadc) !=  HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: can't start reg ADC:",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}
// Starts the regular ADC with interrupt
int _start_reg_ADC_IT(ADC_HandleTypeDef* hadc){ 
  #ifdef SIMPLEFOC_STM32_DEBUG
  SIMPLEFOC_DEBUG("STM32-CS: start reg ADC with IT:",_adcToIndex(hadc)+1);
  #endif
  
  _enable_irq(hadc);
  
  if (HAL_ADC_Start_IT(hadc) !=  HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: can't start reg ADC with IT:",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}

// Calibrated and starts all the ADCs that have been initialized
int _start_ADCs(void* _csparams){
  Stm32CurrentSenseParams* cs_params = (Stm32CurrentSenseParams*)_csparams;
 
  for (int i = 0; i < ADC_COUNT; i++){
    if (ADCEngine.adc_handles[i] != NP){
     
      if(_calibrate_ADC(ADCEngine.adc_handles[i]) == -1) return -1;

      int  adc_index = _adcToIndex(ADCEngine.adc_handles[i]);
      
      // Only if Injected ADC is available
      #ifdef ADC_INJECTED_SOFTWARE_START
      if (ADCEngine.inj_channel_count[adc_index] > 0){
        // There are injected adc channels to be sampled
        if (cs_params->use_adc_interrupt && // if interrupt is required
            ADCEngine.inj_channel_count[adc_index] == ADCEngine.inj_channel_max && // if adc is one with the highest number of injected adc channels
            interrupt_adc == NP // if adc triggering the interrupt is not yet identified
           ){
          // Only one ADC is started with interrupt
          if(_start_inj_ADC_IT(ADCEngine.adc_handles[i]) == -1) return -1;
          interrupt_adc = ADCEngine.adc_handles[i]; // Save adc handle triggering the interrupt
        }else{
          if(_start_inj_ADC(ADCEngine.adc_handles[i]) == -1) return -1;  
        }
      }

      // Start ADC with DMA only if there are several regular channels to be sampled on this ADC
      if (ADCEngine.reg_channel_count[adc_index] > 0){
        if(_start_DMA_ADC(ADCEngine.adc_handles[i]) == -1) return -1;
      }
      #else
      if (ADCEngine.reg_channel_count[adc_index] > 0){
        // There are regular adc channels to be sampled
        if (cs_params->use_adc_interrupt && // if interrupt is required
            ADCEngine.reg_channel_count[adc_index] == ADCEngine.reg_channel_max && // if adc is one with the highest number of regular adc channels
            interrupt_adc == NP // if adc triggering the interrupt is not yet identified
           ){
          // Only one ADC is started with interrupt
          if(_start_reg_ADC_IT(ADCEngine.adc_handles[i]) == -1) return -1;
          interrupt_adc = ADCEngine.adc_handles[i]; // Save adc handle triggering the interrupt
        }else{
          if (ADCEngine.reg_channel_count[adc_index] > 0){
            if(_start_DMA_ADC(ADCEngine.adc_handles[i]) == -1) return -1;
          }  
        }
      }
      #endif
    }
  }

  return 0;
}

// Searches the pin in the sample array and reads value from the adc buffer
uint32_t _read_ADC_pin(int pin){
  for (int i=0;i<ADCEngine.sample_count;i++){
    if (ADCEngine.samples[i].pin == pin) return _read_ADC_sample(i);
  }

  // the pin wasn't found.
  return 0;
}

// Read ADC value for the sample
uint32_t _read_ADC_sample(int i){
  if (ADCEngine.samples[i].buffered){  
    // Sample value is buffered
    return ADCEngine.samples[i].buffer;
  }else{
    // Read ADC value from register
    return _read_ADC_register(i);
  }
}

// Read ADC value for the sample from Injected register, DMA, or ADC register
uint32_t _read_ADC_register(int i){
  if (ADCEngine.samples[i].type == INJECTED_ADC){
    // return injected ADC value from injected ADC register
    return HAL_ADCEx_InjectedGetValue(ADCEngine.samples[i].handle,ADCEngine.samples[i].rank);
  }else{
    // return regular adc values from the DMA buffer
    return _read_ADC_DMA(ADCEngine.samples[i].adc_index,ADCEngine.samples[i].index);
  }
}

// Buffers sampled value if needed (no repetition counter and low current sensing)
void _buffer_ADCs(){
  for (int i=0;i<ADCEngine.sample_count;i++){
    // Samples are flagged as buffered if used for low side current sensing
    if (ADCEngine.samples[i].buffered){
      if (ADCEngine.samples[i].cs_params != nullptr){
        Stm32CurrentSenseParams* cs_params = (Stm32CurrentSenseParams*)ADCEngine.samples[i].cs_params;
          HardwareTimer* pHT = cs_params->timer_handle;
          TIM_HandleTypeDef* htim = pHT->getHandle();
          bool dir = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim);
        // Only buffer adc value if Timer is downcounting  
        if (dir) ADCEngine.samples[i].buffer = _read_ADC_register(i);
      }
    }
  }
}

extern "C" {
  // Only if injected ADC available
  #ifdef ADC_INJECTED_SOFTWARE_START
  #if defined(STM32F4xx) || defined(STM32F7xx)
  void ADC_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(interrupt_adc);
  }
  #endif

  #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
  void ADC1_2_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(interrupt_adc);
  }
  #endif

  #if defined(STM32G4xx) || defined(STM32L4xx)
  #ifdef ADC3
  void ADC3_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(interrupt_adc);
  }
  #endif

  #ifdef ADC4
  void ADC4_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(interrupt_adc);
  }
  #endif

  #ifdef ADC5
  void ADC5_IRQHandler(void)
  {
      HAL_ADC_IRQHandler(interrupt_adc);
  }
  #endif
  #endif
  #endif
}

#endif