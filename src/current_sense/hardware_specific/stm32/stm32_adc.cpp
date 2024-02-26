
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

#include "stm32_adc.h"
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

#ifdef ARDUINO_B_G431B_ESC1
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
int adc_inj_trigger[ADC_COUNT] = {ADC_SOFTWARE_START};
int adc_reg_trigger[ADC_COUNT] = {ADC_SOFTWARE_START};

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

void _configureOPAMPs(void){
#ifdef ARDUINO_B_G431B_ESC1
  // Initialize Opamps
  _configureOPAMP(&hopamp1,OPAMP1);
	_configureOPAMP(&hopamp2,OPAMP2);
	_configureOPAMP(&hopamp3,OPAMP3);
#endif
}

void MX_DMA1_Init(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma_adc, DMA_Channel_TypeDef *channel, uint32_t request) {
  #if defined(STM32G4xx)
  hdma_adc->Instance = channel;
  hdma_adc->Init.Request = request;
  #endif
  #if defined(STM32F4xx)
  hdma_adc->Init.Channel = channel;
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
    SIMPLEFOC_DEBUG("HAL_DMA_Init failed!");
  }
  __HAL_LINKDMA(hadc, DMA_Handle, *hdma_adc);
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

DMA_HandleTypeDef *_dma_get_handle(ADC_TypeDef* Instance){
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

int _add_ADC_pin(uint32_t pin,int32_t trigger, int type){
  PinName pinname = analogInputToPinName(pin);  
  pinmap_pinout(pinname, PinMap_ADC);
  ADC_TypeDef *Instance = (ADC_TypeDef*)pinmap_peripheral(pinname, PinMap_ADC);
  ADC_HandleTypeDef* hadc = _adc_get_handle(Instance);
  int adc_index = _adcToIndex(Instance);   
  adc_handles[adc_index] = hadc; // Store in list of adc handles to be able to loop through later
  
  Stm32ADCSample sample = {};
  sample.Instance  = Instance;
  sample.type      = type; // 0 = inj, 1 = reg
  sample.handle    = hadc;
  sample.pin       = pin;
  sample.channel   = _getADCChannel(pinname);
  sample.adc_index = adc_index;
  sample.trigger   = trigger;
  
  if (type == 0){
    if (adc_inj_channel_count[adc_index] == MAX_INJ_ADC_CHANNELS){
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: max inj channel reached: ", (int) sample.adc_index+1);
      #endif  
    }
    sample.rank = _getInjADCRank(adc_inj_channel_count[adc_index] + 1);
    adc_inj_trigger[adc_index] = trigger;
    sample.index = adc_inj_channel_count[adc_index];
    adc_inj_channel_count[adc_index]++; // Increment total injected channel count for this ADC
  }else{
    if (adc_reg_channel_count[adc_index] == MAX_REG_ADC_CHANNELS){
      #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: max reg channel reached: ", (int) sample.adc_index+1);
      #endif  
    }
    sample.rank = _getRegADCRank(adc_reg_channel_count[adc_index] + 1);
    adc_reg_trigger[adc_index] = trigger;
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

  /*
  #if defined(STM32G4xx) || defined(STM32H5xx) || defined(STM32L4xx) || \
      defined(STM32L5xx) || defined(STM32WBxx)
  if (!IS_ADC_CHANNEL(sample.handle, sample.channel)) {
  #else
  if (!IS_ADC_CHANNEL(sample.channel)) {
  #endif
    return -1;
  }
  */

  samples[sample_count] = sample;
  sample_count++;  

  return sample_count - 1;
}

int _init_ADCs(){
  int status = 0;

  for (int i=0;i<sample_count;i++){
    if (_adc_init(samples[i]) != 0) return status;
    if (samples[i].type == 0 ){
      if (_adc_inj_channel_config(samples[i]) != 0) return status;
    }else{
      if (_adc_reg_channel_config(samples[i]) != 0) return status;
    }
  }

  return 0;
}

int _adc_init(Stm32ADCSample sample)
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
  sample.handle->Init.ScanConvMode = ENABLE;
  sample.handle->Init.ContinuousConvMode = DISABLE;
  
  #if !defined(STM32F1xx) && !defined(STM32F2xx) && !defined(STM32F3xx) && \
      !defined(STM32F4xx) && !defined(STM32F7xx) && !defined(STM32G4xx) && \
      !defined(STM32H5xx) && !defined(STM32H7xx) && !defined(STM32L4xx) &&  \
      !defined(STM32L5xx) && !defined(STM32MP1xx) && !defined(STM32WBxx) || \
      defined(ADC_SUPPORT_2_5_MSPS)
  sample.handle->Init.LowPowerAutoPowerOff  = DISABLE;                       /* ADC automatically powers-off after a conversion and automatically wakes-up when a new conversion is triggered */
  #endif

  sample.handle->Init.DiscontinuousConvMode = DISABLE;
  #if !defined(STM32F1xx) && !defined(ADC1_V2_5)
  sample.handle->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  #endif
  sample.handle->Init.ExternalTrigConv = adc_reg_trigger[sample.adc_index]; // for now
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

  if ( HAL_ADC_Init(sample.handle) != HAL_OK){
  #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init ADC!");
  #endif
    return -1;
  }

  return 0;
}

int _adc_inj_channel_config(Stm32ADCSample sample)
{
  ADC_InjectionConfTypeDef sConfigInjected = {};

  sConfigInjected.ExternalTrigInjecConv = adc_inj_trigger[sample.adc_index];
  #if !defined(STM32F1xx) && !defined(ADC1_V2_5)
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
  #endif
  #ifdef ADC_EXTERNALTRIGINJECCONVEDGE_RISING
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;  
  #endif
  
  sConfigInjected.InjectedSamplingTime = sample.SamplingTime;
  
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

  sConfigInjected.InjectedNbrOfConversion = adc_inj_channel_count[adc_index];
  sConfigInjected.InjectedRank = sample.rank;
  sConfigInjected.InjectedChannel = sample.channel;
  if (HAL_ADCEx_InjectedConfigChannel(sample.handle, &sConfigInjected) != HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init injected channel: ", (int) sConfigInjected.InjectedChannel);
    #endif
    return -1;
  }

  return 0;

}

int _adc_reg_channel_config(Stm32ADCSample sample)
{
  ADC_ChannelConfTypeDef  AdcChannelConf = {};
  AdcChannelConf.Channel = sample.channel;                          /* Specifies the channel to configure into ADC */
  AdcChannelConf.Rank = sample.rank;               /* Specifies the rank in the regular group sequencer */
  AdcChannelConf.SamplingTime = sample.SamplingTime;                     /* Sampling time value to be set for the selected channel */
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
      SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init regular channel: ", (int) AdcChannelConf.Channel);
    #endif
    return -1;
  }

  return 0;
  
}

// Calibrates the ADC if initialized and not already started
int _calibrate_ADC(ADC_HandleTypeDef* hadc){
  if (hadc->Instance == 0) return 0; // ADC not initialized
  if (LL_ADC_IsEnabled(hadc->Instance)) return 0; // ADC already started


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

  #endif

  return 0;
  
}


// Starts the ADC if initialized and not already started
int _start_ADC(ADC_HandleTypeDef* hadc){

  if (hadc->Instance == 0) return 0; // ADC not initialized
  if (LL_ADC_IsEnabled(hadc->Instance)) return 0; // ADC already started
  
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

  _configureOPAMPs();

  for (int i = 0; i < ADC_COUNT; i++){
    if (adc_handles[i] != NP){
      int adc_index = _adcToIndex(adc_handles[i]->Instance);
    
      status = _calibrate_ADC(adc_handles[i]);
      if (status!=0) return status;

      if ((adc_handles[i])->Instance == ADC1) status = _start_ADC_IT(adc_handles[i]);
      else status = _start_ADC(adc_handles[i]);
      if (status!=0) return status;
     
      status = _start_DMA(adc_handles[i]);
    
    }
  }

  return 0;
}

int _start_DMA(ADC_HandleTypeDef* hadc){
  int status = 0;
  #if defined(__HAL_RCC_DMAMUX1_CLK_ENABLE)
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  #endif 
  __HAL_RCC_DMA1_CLK_ENABLE();
  
  int adc_index = _adcToIndex(hadc->Instance);
  
  DMA_HandleTypeDef* hdma_adc = _dma_get_handle(hadc->Instance);
  DMA_Channel_TypeDef* dma_channel = _getDMAChannel(adc_index); 
  uint32_t dma_request = _getDMARequest(adc_index);
  MX_DMA1_Init(hadc,hdma_adc,dma_channel, dma_request);

  uint32_t* address = (uint32_t*)(adc_reg_val) + (MAX_REG_ADC_CHANNELS/2*adc_index); // Calculate the address for the right row in the array
  if (HAL_ADC_Start_DMA(hadc,  address , adc_reg_channel_count[adc_index]) != HAL_OK) 
  {
    SIMPLEFOC_DEBUG("DMA read init failed");
    return -1;
  }
  return 0;
}

// Writes Injected ADC values to the adc buffer
void _read_ADC(ADC_HandleTypeDef* hadc){

  if (hadc->Instance == 0) return; // skip if ADC not initialized
  if (!LL_ADC_IsEnabled(hadc->Instance)) return; // skip if ADC not started

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
uint32_t _read_adc_buf(int adc_index,int index){
  return adc_inj_val[adc_index][index];
}

// Reads value from the adc buffer when adc index and rank are known
uint32_t _read_adc_dma(int adc_index,int index){
  return adc_reg_val[adc_index][index];
}

// Reads value from the adc buffer when the sample number is known
uint32_t _read_adc_sample(int index){
  if (samples[index].type == 0){
    return _read_adc_buf(samples[index].adc_index,samples[index].index);
  }else{
    return _read_adc_dma(samples[index].adc_index,samples[index].index);
  }
}

// Searches the pin on the samples array and reads value from the adc buffer
uint32_t _read_adc_pin(int pin){
  for (int i=0;i<sample_count;i++){
    if (samples[i].pin == pin) return _read_adc_sample(i);
  }

  return 0;
}

uint32_t _getDMARequest(int index){
  switch(index){
    #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
    case 0:
      return DMA_REQUEST_ADC1;
    #ifdef DMA_REQUEST_ADC2
    case 1:
      return DMA_REQUEST_ADC2;
    #endif
    #ifdef DMA_REQUEST_ADC3
    case 2:
      return DMA_REQUEST_ADC3;
    #endif
    #ifdef DMA_REQUEST_ADC4
    case 3:
      return DMA_REQUEST_ADC4;
    #endif
    #ifdef DMA_REQUEST_ADC5
    case 4:
      return DMA_REQUEST_ADC5;
    #endif
    #endif
    default:
      return 0;
  }
}

DMA_Channel_TypeDef *_getDMAChannel(int index){
  switch(index){
    #if defined(STM32F4xx)
    case 0:   
      return DMA_CHANNEL_0;
    #ifdef DMA_CHANNEL_0
    case 1:
      return DMA_CHANNEL_1;
    #endif
    #ifdef DMA_CHANNEL_2
    case 2:
      return DMA_CHANNEL_2;
    #endif
    #ifdef DMA_CHANNEL_3
    case 3:
      return DMA_CHANNEL_3;
    #endif
    #ifdef DMA_CHANNEL_4
    case 4:
      return DMA_CHANNEL_4;
    #endif
    #endif
    #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
    case 0:   
      return DMA1_Channel1;
    #ifdef DMA1_Channel2
    case 1:
      return DMA1_Channel2;
    #endif
    #ifdef DMA1_Channel3
    case 2:
      return DMA1_Channel3;
    #endif
    #ifdef DMA1_Channel4
    case 3:
      return DMA1_Channel4;
    #endif
    #ifdef DMA1_Channel5
    case 4:
      return DMA1_Channel5;
    #endif
    #endif
    default:
      return 0;
  }
}

uint32_t _getInjADCRank(int index)
{
  switch(index){
    case 1:
      return ADC_INJECTED_RANK_1;
    case 2:
      return ADC_INJECTED_RANK_2;
    case 3:
      return ADC_INJECTED_RANK_3;
    case 4:
      return ADC_INJECTED_RANK_4;
    default:
      return 0;
  }
}

uint32_t _getRegADCRank(int index)
{
  switch(index){
    #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
    case 1:
      return ADC_REGULAR_RANK_1;
    case 2:
      return ADC_REGULAR_RANK_2;
    case 3:
      return ADC_REGULAR_RANK_3;
    case 4:
      return ADC_REGULAR_RANK_4;
    case 5:
      return ADC_REGULAR_RANK_5;
    case 6:
      return ADC_REGULAR_RANK_6;
    case 7:
      return ADC_REGULAR_RANK_7;
    case 8:
      return ADC_REGULAR_RANK_8;
    case 9:
      return ADC_REGULAR_RANK_9;
    case 10:
      return ADC_REGULAR_RANK_10;
    case 11:
      return ADC_REGULAR_RANK_11;
    case 12:
      return ADC_REGULAR_RANK_12;
    case 13:
      return ADC_REGULAR_RANK_13;
    case 14:
      return ADC_REGULAR_RANK_14;
    case 15:
      return ADC_REGULAR_RANK_15;
    case 16:
      return ADC_REGULAR_RANK_16;
    #endif
    default:
      return index + 1;
  }
}

/* Exported Functions */
/**
  * @brief  Return ADC HAL channel linked to a PinName
  * @param  pin: PinName
  * @retval Valid HAL channel
  */
uint32_t _getADCChannel(PinName pin)
{
  uint32_t function = pinmap_function(pin, PinMap_ADC);
  uint32_t channel = 0;
  switch (STM_PIN_CHANNEL(function)) {
#ifdef ADC_CHANNEL_0
    case 0:
      channel = ADC_CHANNEL_0;
      break;
#endif
    case 1:
      channel = ADC_CHANNEL_1;
      break;
    case 2:
      channel = ADC_CHANNEL_2;
      break;
    case 3:
      channel = ADC_CHANNEL_3;
      break;
    case 4:
      channel = ADC_CHANNEL_4;
      break;
    case 5:
      channel = ADC_CHANNEL_5;
      break;
    case 6:
      channel = ADC_CHANNEL_6;
      break;
    case 7:
      channel = ADC_CHANNEL_7;
      break;
    case 8:
      channel = ADC_CHANNEL_8;
      break;
    case 9:
      channel = ADC_CHANNEL_9;
      break;
    case 10:
      channel = ADC_CHANNEL_10;
      break;
    case 11:
      channel = ADC_CHANNEL_11;
      break;
    case 12:
      channel = ADC_CHANNEL_12;
      break;
    case 13:
      channel = ADC_CHANNEL_13;
      break;
    case 14:
      channel = ADC_CHANNEL_14;
      break;
    case 15:
      channel = ADC_CHANNEL_15;
      break;
#ifdef ADC_CHANNEL_16
    case 16:
      channel = ADC_CHANNEL_16;
      break;
#endif
    case 17:
      channel = ADC_CHANNEL_17;
      break;
#ifdef ADC_CHANNEL_18
    case 18:
      channel = ADC_CHANNEL_18;
      break;
#endif
#ifdef ADC_CHANNEL_19
    case 19:
      channel = ADC_CHANNEL_19;
      break;
#endif
#ifdef ADC_CHANNEL_20
    case 20:
      channel = ADC_CHANNEL_20;
      break;
    case 21:
      channel = ADC_CHANNEL_21;
      break;
    case 22:
      channel = ADC_CHANNEL_22;
      break;
#ifdef ADC_CHANNEL_23
    case 23:
      channel = ADC_CHANNEL_23;
      break;
#ifdef ADC_CHANNEL_24
    case 24:
      channel = ADC_CHANNEL_24;
      break;
    case 25:
      channel = ADC_CHANNEL_25;
      break;
    case 26:
      channel = ADC_CHANNEL_26;
      break;
#ifdef ADC_CHANNEL_27
    case 27:
      channel = ADC_CHANNEL_27;
      break;
    case 28:
      channel = ADC_CHANNEL_28;
      break;
    case 29:
      channel = ADC_CHANNEL_29;
      break;
    case 30:
      channel = ADC_CHANNEL_30;
      break;
    case 31:
      channel = ADC_CHANNEL_31;
      break;
#endif
#endif
#endif
#endif
     default:
      _Error_Handler("ADC: Unknown adc channel", (int)(STM_PIN_CHANNEL(function)));
      break;
  }
  return channel;
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