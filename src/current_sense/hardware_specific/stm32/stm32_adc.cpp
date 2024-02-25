
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

#include "stm32_adc.h"
#include "../../../communication/SimpleFOCDebug.h"

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

#ifdef ARDUINO_B_G431B_ESC1
OPAMP_HandleTypeDef hopamp1;
OPAMP_HandleTypeDef hopamp2;
OPAMP_HandleTypeDef hopamp3;
#endif

// array of values of 4 injected channels per adc instance (5)
uint32_t adc_val[ADC_COUNT][4]={0};
Stm32ADCSample samples[ADC_COUNT * 4] = {};
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

int _add_ADC_pin(uint32_t pin,int32_t trigger){
  PinName pinname = analogInputToPinName(pin);  
  pinmap_pinout(pinname, PinMap_ADC);
  ADC_TypeDef *Instance = (ADC_TypeDef*)pinmap_peripheral(pinname, PinMap_ADC);
  ADC_HandleTypeDef* hadc = _adc_get_handle(Instance);
  int adc_index = _adcToIndex(Instance);   
  adc_handles[adc_index] = hadc; // Store in list of adc handles to be able to loop through later
  
  Stm32ADCSample sample = {};
  sample.Instance  = Instance;
  sample.handle    = hadc;
  sample.pin       = pin;
  sample.rank      = _getInjADCRank(adc_inj_channel_count[adc_index] + 1);
  sample.channel   = _getADCChannel(pinname);
  sample.adc_index = adc_index;
  sample.index     = adc_inj_channel_count[adc_index];
  sample.trigger   = trigger;

  samples[sample_count] = sample;
  
  adc_inj_channel_count[adc_index]++; // Increment total channel count for this ADC
  sample_count++;  

  return sample_count - 1;
}

int _init_ADCs(){
  int status = 0;
  for (int i=0;i<sample_count;i++){
    if (_adc_init(samples[i]) != 0) return status;
    if (_adc_channel_config(samples[i]) != 0) return status;
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

  sample.handle->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
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
  sample.handle->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  #endif
  sample.handle->Init.ExternalTrigConv = ADC_SOFTWARE_START; // for now
  #ifdef ADC_DATAALIGN_RIGHT
  sample.handle->Init.DataAlign = ADC_DATAALIGN_RIGHT;
  #endif
  #if !defined(STM32F0xx) && !defined(STM32L0xx)
  sample.handle->Init.NbrOfConversion = 1;
  #endif
  sample.handle->Init.DMAContinuousRequests = DISABLE;
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

int _adc_channel_config(Stm32ADCSample sample)
{
  ADC_InjectionConfTypeDef sConfigInjected = {};

  sConfigInjected.ExternalTrigInjecConv = sample.trigger;
  #ifdef ADC_EXTERNALTRIGINJECCONV_EDGE_RISING
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
  #endif
  #ifdef ADC_EXTERNALTRIGINJECCONVEDGE_RISING
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;  
  #endif
  
  #if defined(STM32F1xx)
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_1CYCLE_5
  #endif
  #if defined(STM32F4xx)
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_3CYCLES;
  #endif
  #if defined(STM32G4xx) || defined(STM32L4xx)
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_2CYCLES_5;
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

  return 0;
#endif
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
      status = _calibrate_ADC(adc_handles[i]);
      if (status!=0) return status;

      if ((adc_handles[i])->Instance == ADC1) status = _start_ADC_IT(adc_handles[i]);
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
  int channel_count = adc_inj_channel_count[_adcToIndex(hadc)];
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

// Reads value from the adc buffer when adc index and rank are known
uint32_t _read_adc_val(int adc_index,int index){
  return adc_val[adc_index][index];
}

// Reads value from the adc buffer when the sample number is known
uint32_t _read_adc_sample(int index){
  return _read_adc_val(samples[index].adc_index,samples[index].index);
}

// Searches the pin on the samples array and reads value from the adc buffer
uint32_t _read_adc_pin(int pin){
  for (int i=0;i<sample_count;i++){
    if (samples[i].pin == pin) return _read_adc_val(samples[i].adc_index,samples[i].index);
  }

  return 0;
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