
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

#include "stm32_adc.h"
#include "stm32_utils.h"
#include "../../../communication/SimpleFOCDebug.h"

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
#ifdef ADC2
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc2;
#endif
#ifdef ADC3
ADC_HandleTypeDef hadc3;
DMA_HandleTypeDef hdma_adc3;
#endif
#ifdef ADC4
ADC_HandleTypeDef hadc4;
DMA_HandleTypeDef hdma_adc4;
#endif
#ifdef ADC5
ADC_HandleTypeDef hadc5;
DMA_HandleTypeDef hdma_adc5;
#endif

#ifdef OPAMP
OPAMP_HandleTypeDef hopamp1;
OPAMP_HandleTypeDef hopamp2;
OPAMP_HandleTypeDef hopamp3;
#endif

#define MAX_REG_ADC_CHANNELS 16
#define MAX_INJ_ADC_CHANNELS 4

// array of values of 4 injected channels per adc instance (5)
uint16_t adc_reg_val[ADC_COUNT][MAX_REG_ADC_CHANNELS]={0};
uint16_t adc_inj_val[ADC_COUNT][MAX_INJ_ADC_CHANNELS]={0};
Stm32ADCSample samples[ADC_COUNT * (MAX_REG_ADC_CHANNELS+MAX_INJ_ADC_CHANNELS)] = {}; // The maximum number of sample is the number of ADC * 4 injected + 16 regular
int sample_count = 0;

ADC_HandleTypeDef* adc_handles[ADC_COUNT] = {NP};
int adc_inj_channel_count[ADC_COUNT] = {0};
int adc_reg_channel_count[ADC_COUNT] = {0};
int adc_inj_trigger[ADC_COUNT] = {0};
int adc_reg_trigger[ADC_COUNT] = {0};

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

DMA_HandleTypeDef *_get_DMA_handle(ADC_TypeDef* Instance){
  if (Instance == ADC1) return &hdma_adc1;
  #ifdef ADC2
  else if (Instance == ADC2) return &hdma_adc2; 
  #endif
  #ifdef ADC3
  else if (Instance == ADC3) return &hdma_adc3;
  #endif
  #ifdef ADC4
  else if (Instance == ADC4) return &hdma_adc4;
  #endif
  #ifdef ADC5
  else if (Instance == ADC5) return &hdma_adc5;
  #endif
  else return nullptr;
}

#ifdef OPAMP
int _init_OPAMP(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *OPAMPx_Def){
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
    SIMPLEFOC_DEBUG("STM32-CS: ERR: OPAMP init failed!");
    return -1;
  }

  if (HAL_OPAMP_SelfCalibrate(hopamp) != HAL_OK)
  {
    SIMPLEFOC_DEBUG("STM32-CS: ERR: OPAMP calibrate failed!");
    return -1;
  }

  if (HAL_OPAMP_Start(hopamp) != HAL_OK)
  {
    SIMPLEFOC_DEBUG("STM32-CS: ERR: OPAMP start failed!");
    return -1;
  }

  return 0;
}

int _init_OPAMPs(void){
  // Initialize Opamps
  if (_init_OPAMP(&hopamp1,OPAMP1) == -1) return -1;
	if (_init_OPAMP(&hopamp2,OPAMP2) == -1) return -1;
	if (_init_OPAMP(&hopamp3,OPAMP3) == -1) return -1;
  return 0;
}

#endif

int _add_inj_ADC_sample(uint32_t pin,int32_t trigger){
  return _add_ADC_sample(pin,trigger,0);
}

int _add_reg_ADC_sample(uint32_t pin){
  return _add_ADC_sample(pin,ADC_SOFTWARE_START,1);
}

int _add_ADC_sample(uint32_t pin,int32_t trigger,int type){
  PinName pinname = analogInputToPinName(pin);  
  pinmap_pinout(pinname, PinMap_ADC);
  ADC_TypeDef *Instance = (ADC_TypeDef*)pinmap_peripheral(pinname, PinMap_ADC);
  ADC_HandleTypeDef* hadc = _get_ADC_handle(Instance);
  int adc_index = _adcToIndex(Instance);   
  adc_handles[adc_index] = hadc; // Store in list of adc handles to be able to loop through later
  
  Stm32ADCSample sample = {};
  sample.Instance  = Instance;
  sample.type      = type; // 0 = inj, 1 = reg
  sample.handle    = hadc;
  sample.pin       = pin;
  sample.channel   = _getADCChannel(pinname);
  sample.adc_index = adc_index;
  
  if (type == 0){
    if (adc_inj_channel_count[adc_index] == MAX_INJ_ADC_CHANNELS){
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: max inj channel reached: ", (int) sample.adc_index+1);
      #endif  
    }
    adc_inj_trigger[adc_index] = trigger;
    sample.rank = _getInjADCRank(adc_inj_channel_count[adc_index] + 1);
    sample.index = adc_inj_channel_count[adc_index];
    adc_inj_channel_count[adc_index]++; // Increment total injected channel count for this ADC
  }else{
    if (adc_reg_channel_count[adc_index] == MAX_REG_ADC_CHANNELS){
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: max reg channel reached: ", (int) sample.adc_index+1);
      #endif  
    }
    adc_reg_trigger[adc_index] = trigger;
    sample.rank = _getRegADCRank(adc_reg_channel_count[adc_index] + 1);
    sample.index = adc_reg_channel_count[adc_index];
    adc_reg_channel_count[adc_index]++; // Increment total regular channel count for this ADC
  }
  
  #if defined(STM32F1xx)
  sample.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  #endif
  #if defined(STM32F4xx)
  sample.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  #endif
  #if defined(STM32G4xx) || defined(STM32L4xx)
  sample.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  #endif

  samples[sample_count] = sample;
  sample_count++;  

  return sample_count - 1; // Return index of the sample
}

int _init_ADCs(){
  int status = 0;

  for (int i=0;i<sample_count;i++){
    if (_init_ADC(samples[i]) == -1) return -1;
    if (samples[i].type == 0 ){
      if (_add_inj_ADC_channel_config(samples[i]) == -1) return -1;
    }else{
      if (_add_reg_ADC_channel_config(samples[i]) == -1) return -1;
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
  
  #ifdef ADC_CLOCK_SYNC_PCLK_DIV4
  sample.handle->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  #endif
  #ifdef ADC_RESOLUTION_12B
  sample.handle->Init.Resolution = ADC_RESOLUTION_12B;
  #endif

  #if defined(STM32F1xx)
  sample.handle->Init.ScanConvMode = ADC_SCAN_ENABLE; 
  #else
  sample.handle->Init.ScanConvMode = ENABLE;
  #endif

  sample.handle->Init.ContinuousConvMode = ENABLE;
  
  #if !defined(STM32F1xx) && !defined(STM32F2xx) && !defined(STM32F3xx) && \
      !defined(STM32F4xx) && !defined(STM32F7xx) && !defined(STM32G4xx) && \
      !defined(STM32H5xx) && !defined(STM32H7xx) && !defined(STM32L4xx) &&  \
      !defined(STM32L5xx) && !defined(STM32MP1xx) && !defined(STM32WBxx) || \
      defined(ADC_SUPPORT_2_5_MSPS)
  sample.handle->Init.LowPowerAutoPowerOff  = DISABLE;                       /* ADC automatically powers-off after a conversion and automatically wakes-up when a new conversion is triggered */
  #endif

  sample.handle->Init.DiscontinuousConvMode = DISABLE;
  sample.handle->Init.ExternalTrigConv = adc_reg_trigger[sample.adc_index];
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
  sample.handle->Init.NbrOfConversion = max(1,adc_reg_channel_count[sample.adc_index]); // Minimum 1 for analogread to work
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

  // Init DMA only if there are regular channels to be sampled on this ADC
  if (adc_reg_channel_count[sample.adc_index] > 0){
    if (_init_DMA(sample.handle) == -1) return -1;
  }

  if ( HAL_ADC_Init(sample.handle) != HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init ADC!");
    #endif
    return -1;
  }

  return 0;
}

int _init_DMA(ADC_HandleTypeDef *hadc){
  #if defined(__HAL_RCC_DMAMUX1_CLK_ENABLE)
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  #endif 

  #ifdef __HAL_RCC_DMA1_CLK_ENABLE
  __HAL_RCC_DMA1_CLK_ENABLE();
  #endif
  #ifdef __HAL_RCC_DMA2_CLK_ENABLE
  __HAL_RCC_DMA2_CLK_ENABLE();
  #endif
  
  int adc_index = _adcToIndex(hadc->Instance);

  #ifdef STM32F1xx
    if (hadc->Instance == ADC2){
      SIMPLEFOC_DEBUG("STM32-CS: ERR: DMA can't work with ADC2");
      return 0;
    }
  #endif
  
  DMA_HandleTypeDef* hdma_adc = _get_DMA_handle(hadc->Instance);
  
  #if defined(STM32G4xx) || defined(STM32L4xx)
  hdma_adc->Instance = _getDMAChannel(adc_index);
  hdma_adc->Init.Request = _getDMARequest(adc_index);
  #endif
  #if defined(STM32F4xx)
  hdma_adc->Instance = _getDMAStream(adc_index);
  hdma_adc->Init.Channel = _getDMAChannel(adc_index);
  #endif
  #if defined(STM32F1xx)
  hdma_adc->Instance = _getDMAChannel(adc_index);
  #endif

  hdma_adc->Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc->Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc->Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc->Init.Mode = DMA_CIRCULAR;
  hdma_adc->Init.Priority = DMA_PRIORITY_LOW;

  HAL_DMA_DeInit(hdma_adc);
  if (HAL_DMA_Init(hdma_adc) != HAL_OK)
  {
    SIMPLEFOC_DEBUG("STM32-CS: ERR: DMA Init failed!",_adcToIndex(hadc)+1);
    return -1;
  }
  __HAL_LINKDMA(hadc, DMA_Handle, *hdma_adc);

  return 0;
}

int _add_inj_ADC_channel_config(Stm32ADCSample sample)
{
  ADC_InjectionConfTypeDef sConfigInjected = {};

  #if defined(STM32F4xx) 
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;
  #endif
  
  #if defined(STM32G4xx) || defined(STM32L4xx)
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

  sConfigInjected.ExternalTrigInjecConv   = adc_inj_trigger[sample.adc_index];
  sConfigInjected.InjectedSamplingTime    = sample.SamplingTime;
  sConfigInjected.InjectedNbrOfConversion = adc_inj_channel_count[adc_index];
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

int _add_reg_ADC_channel_config(Stm32ADCSample sample)
{
  ADC_ChannelConfTypeDef  AdcChannelConf = {};
  AdcChannelConf.Channel      = sample.channel;            /* Specifies the channel to configure into ADC */
  AdcChannelConf.Rank         = sample.rank;               /* Specifies the rank in the regular group sequencer */
  AdcChannelConf.SamplingTime = sample.SamplingTime;       /* Sampling time value to be set for the selected channel */
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
  if (_is_enabled_ADC(hadc)) return 0; // ADC already enabled

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


// Starts the injected ADC
int _start_ADC(ADC_HandleTypeDef* hadc){
  if (HAL_ADCEx_InjectedStart(hadc) !=  HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: can't start inj ADC :",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}

// Starts the regular ADC with interrupt
int _start_ADC_IT(ADC_HandleTypeDef* hadc){      
  // enable interrupt
  #if defined(STM32F4xx)
  HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC_IRQn);
  #endif

  #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
  if(hadc->Instance == ADC1) {
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
    SIMPLEFOC_DEBUG("STM32-CS: ERR: can't start inj ADC with IT:",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}


// Triggers the software start of regular ADC
void _start_reg_conversion_ADCs(void){
  for (int i = 0; i < ADC_COUNT; i++){
    if (adc_handles[i] != NP){
      if (adc_reg_channel_count[i] > 0 && adc_reg_trigger[i] == ADC_SOFTWARE_START){
        #if defined(STM32F1xx)
        SET_BIT(adc_handles[i]->Instance->CR2, (ADC_CR2_SWSTART | ADC_CR2_EXTTRIG));
        #endif
        #if defined(STM32F4xx)
        LL_ADC_REG_StartConversionSWStart(adc_handles[i]->Instance);
        #endif
        #if defined(STM32G4xx) || defined(STM32L4xx) 
        LL_ADC_REG_StartConversion(adc_handles[i]->Instance);
        #endif
      }
    }
  }
}

// Calibrated and starts all the ADCs that have been initialized
int _start_ADCs(void){
  int status = 0;

  for (int i = 0; i < ADC_COUNT; i++){
    if (adc_handles[i] != NP){
     
      if(_calibrate_ADC(adc_handles[i]) == -1) return -1;

      // For now only ADC1 is started with interrupt
      if ((adc_handles[i])->Instance == ADC1){
        if(_start_ADC_IT(adc_handles[i]) == -1) return -1;
      }else{
        if(_start_ADC(adc_handles[i]) == -1) return -1;
      }
     
      if(_start_DMA(adc_handles[i]) == -1) return -1;
    
    }
  }

  return 0;
}

int _start_DMA(ADC_HandleTypeDef* hadc){

  if (hadc->DMA_Handle == 0) return 0; // Skip DMA start if no DMA handle
  int adc_index = _adcToIndex(hadc->Instance);

  // Start DMA only if there are regular channels to be sampled on this ADC
  if (adc_reg_channel_count[adc_index] == 0) return 0;

  // Calculate the address for the right row in the array    
  uint32_t* address = (uint32_t*)(adc_reg_val) + (MAX_REG_ADC_CHANNELS/2*adc_index); 
  if (HAL_ADC_Start_DMA(hadc,  address , adc_reg_channel_count[adc_index]) != HAL_OK) 
  {
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: DMA start failed",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}

// Writes Injected ADC values to the adc buffer
void _read_ADC(ADC_HandleTypeDef* hadc){

  int adc_index = _adcToIndex(hadc);
  int channel_count = adc_inj_channel_count[_adcToIndex(hadc)];
  for(int i=0;i<channel_count;i++){
    adc_inj_val[adc_index][i] = HAL_ADCEx_InjectedGetValue(hadc,_getInjADCRank(i+1));
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

// Reads value from the adc buffer when adc index and rank are known
uint32_t _read_ADC_buf(int adc_index,int index){
  return adc_inj_val[adc_index][index];
}

// Reads value from the adc buffer when adc index and rank are known
uint32_t _read_DMA_buf(int adc_index,int index){
  return adc_reg_val[adc_index][index];
}

// Reads value from the adc buffer when the sample number is known
uint32_t _read_ADC_sample(int index){
  if (samples[index].type == 0){
    return _read_ADC_buf(samples[index].adc_index,samples[index].index);
  }else{
    return _read_DMA_buf(samples[index].adc_index,samples[index].index);
  }
}

// Searches the pin on the samples array and reads value from the adc buffer
uint32_t _read_ADC_pin(int pin){
  for (int i=0;i<sample_count;i++){
    if (samples[i].pin == pin) return _read_ADC_sample(i);
  }

  // the pin wasn't found.
  return 0;
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