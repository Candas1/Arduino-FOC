#include "stm32_dma.h"

DMA_HandleTypeDef hdma_adc1;
#ifdef ADC2
DMA_HandleTypeDef hdma_adc2;
#endif
#ifdef ADC3
DMA_HandleTypeDef hdma_adc3;
#endif
#ifdef ADC4
DMA_HandleTypeDef hdma_adc4;
#endif
#ifdef ADC5
DMA_HandleTypeDef hdma_adc5;
#endif

extern Stm32ADCEngine ADCEngine;
uint16_t adc_dma_buf[ADC_COUNT][MAX_REG_ADC_CHANNELS]={0};

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
  #if defined(STM32F4xx) || defined(STM32F7xx)
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

int _start_DMA_ADC(ADC_HandleTypeDef* hadc){
  #ifdef SIMPLEFOC_STM32_DEBUG
  SIMPLEFOC_DEBUG("STM32-CS: start DMA for ADC ",_adcToIndex(hadc)+1);
  #endif  

  if (hadc->DMA_Handle == 0) return 0; // Skip DMA start if no DMA handle
  int adc_index = _adcToIndex(hadc->Instance);

  // Calculate the address for the right row in the array    
  uint32_t* address = (uint32_t*)(adc_dma_buf) + (MAX_REG_ADC_CHANNELS/2*adc_index); 
  if (HAL_ADC_Start_DMA(hadc,  address , ADCEngine.reg_channel_count[adc_index] ) != HAL_OK) 
  {
    #ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: can't start DMA for ADC ",_adcToIndex(hadc)+1);
    #endif
    return -1;
  }
  return 0;
}

uint32_t _read_ADC_DMA(int adc_index,int index){
  return (uint32_t) adc_dma_buf[adc_index][index];
}


#if defined(STM32F0xx) || defined(STM32L0xx) || defined(STM32L1xx)
  // DMA V1_0 or V1_1 OK
  // {Instance, channel, request}
  // check V1_1 hal drivers
  #ifdef ADC
  #define DMA_ADC { DMA1_Channel1, 0, 0}
  #endif
#endif

#if defined(STM32F1xx)
// DMA V1_0 OK
// {Instance, channel, request}
  #ifdef ADC1
  #define DMA_ADC1 { DMA1_Channel1, 0, 0}
  #endif
  #ifdef ADC2
  #define DMA_ADC2 {             0, 0, 0}
  #endif
  #ifdef ADC3
  #define DMA_ADC3 { DMA2_Channel5, 0, 0}
  #endif
#endif
  
#if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
  // DMA V2_0 OK
  // {Instance, channel, request}
  #ifdef ADC1
  #define DMA_ADC1 { DMA2_Stream0, DMA_CHANNEL_0, 0}
  #endif
  #ifdef ADC2
  #define DMA_ADC2 { DMA2_Stream2, DMA_CHANNEL_1, 0 }
  #endif
  #ifdef ADC3
  #define DMA_ADC3 { DMA2_Stream1, DMA_CHANNEL_2, 0 }
  #endif
#endif

#if defined(STM32C0xx) || defined(STM32G0xx) || defined(STM32G4xx) || defined(STM32H7xx) || defined(STM32U0xx)
  // DMA V1_3 OK
  // {Instance, channel, request}
  #ifdef ADC1
  #define DMA_ADC1 { DMA1_Channel1, 0, DMA_REQUEST_ADC1}
  #endif
  #ifdef ADC2
  #define DMA_ADC2 { DMA1_Channel2, 0, DMA_REQUEST_ADC2 }
  #endif
  #ifdef ADC3
  #define DMA_ADC3 { DMA1_Channel3, 0, DMA_REQUEST_ADC3 }
  #endif
  #ifdef ADC4
  #define DMA_ADC4 { DMA1_Channel4, 0, DMA_REQUEST_ADC4 }
  #endif
  #ifdef ADC5
  #define DMA_ADC5 { DMA1_Channel5, 0, DMA_REQUEST_ADC5 }
  #endif
#endif

#if defined(STM32L4xx)
  // V1_1 V1_2 V1_3 to be checked, check V1_2 hal driver
  // {Instance, channel, request}
  #ifdef ADC1
  #define DMA_ADC1 { DMA1_Channel1, 0, DMA_REQUEST_0}
  #endif
  #ifdef ADC2
  #define DMA_ADC2 { DMA1_Channel2, 0, DMA_REQUEST_0 }
  #endif
  #ifdef ADC3
  #define DMA_ADC3 { DMA1_Channel3, 0, DMA_REQUEST_0 }
  #endif
#endif

#if defined(STM32L5xx)
  // V2_0 to be checked
  // {Instance, channel, request}
  #ifdef ADC1
  #define DMA_ADC1 { DMA1_Channel1, 0, DMA_REQUEST_0}
  #endif
  #ifdef ADC2
  #define DMA_ADC2 { DMA1_Channel2, 0, DMA_REQUEST_0 }
  #endif
  #ifdef ADC3
  #define DMA_ADC3 { DMA1_Channel3, 0, DMA_REQUEST_0 }
  #endif
#endif

uint32_t _getDMARequest(int index){
  switch(index){
    #if defined(STM32L4xx)
    case 0:
    case 1:
    case 2:
      return DMA_REQUEST_0;
    #endif
    #if defined(STM32G4xx)
    #ifdef DMA_REQUEST_ADC1
    case 0:
      return DMA_REQUEST_ADC1;
    #endif
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

#if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
DMA_Stream_TypeDef *_getDMAStream(int index){
  switch(index){
    case 0:
      return DMA2_Stream0;
    case 1:
      return DMA2_Stream2;
    case 2:
      return DMA2_Stream1;
    default:
      return 0;
  }
}
#endif


#if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
uint32_t _getDMAChannel(int index){
#else
DMA_Channel_TypeDef *_getDMAChannel(int index){
#endif
  switch(index){
    #if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
    case 0:   
      return DMA_CHANNEL_0;
    case 1:
      return DMA_CHANNEL_1;
    case 2:
      return DMA_CHANNEL_2;
    #endif
    
    #if defined(STM32F1xx)
    #ifdef DMA1_Channel1
    case 0:
      return DMA1_Channel1;
    #endif
    case 1:
      return 0; // Not available for ADC2
    #ifdef DMA2_Channel5
    case 2:
      return DMA2_Channel5;
    #endif
    #endif

    #if defined(STM32G4xx) || defined(STM32L4xx)
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