
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
ADC_HandleTypeDef* adc_handles[ADC_COUNT] = {NP};
int adc_channel_count[ADC_COUNT] = {0};
int adc_channel_rank[ADC_COUNT] = {0};

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

int _adc_init(ADC_TypeDef* Instance, ADC_HandleTypeDef* hadc)
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
  
  return 0;
}

Stm32ADCSample _adc_channel_config(ADC_HandleTypeDef* hadc, int pin, int32_t trigger)
{
  ADC_InjectionConfTypeDef sConfigInjected = {};
  Stm32ADCSample sample = {NP,NP,NP};

  sConfigInjected.ExternalTrigInjecConv = trigger;
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
  

  int adc_index = _adcToIndex(hadc);

  sConfigInjected.InjectedNbrOfConversion = adc_channel_count[adc_index];
  sConfigInjected.InjectedRank = _getInjADCRank(adc_channel_rank[adc_index] + 1);
  sConfigInjected.InjectedChannel = _getADCChannel(analogInputToPinName(pin));
  if (HAL_ADCEx_InjectedConfigChannel(hadc, &sConfigInjected) != HAL_OK){
    #ifdef SIMPLEFOC_STM32_DEBUG
      SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init injected channel: ", (int) sConfigInjected.InjectedChannel);
    #endif
    return sample;
  }

  sample.adc_handle = hadc;
  sample.adc_rank   = sConfigInjected.InjectedRank;
  sample.adc_index  = adc_channel_rank[adc_index];
  adc_channel_rank[adc_index]++;

  return sample;

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

uint32_t _read_inj_val(int adc_index,int index){
  return adc_val[adc_index][index];
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