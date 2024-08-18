#ifndef STM32_DMA_DEF
#define STM32_DMA_DEF
#include "Arduino.h"
#include "../../../communication/SimpleFOCDebug.h"
#include "stm32_adc.h"
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

int _init_DMA(ADC_HandleTypeDef *hadc);
int _start_DMA_ADC(ADC_HandleTypeDef* hadc);
uint32_t _read_ADC_DMA(int adc_index,int index);
uint32_t _getDMARequest(int index);
#if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
uint32_t _getDMAChannel(int index);
#else
DMA_Channel_TypeDef *_getDMAChannel(int index);
#endif
#if defined(STM32F4xx) || defined(STM32F7xx)
DMA_Stream_TypeDef *_getDMAStream(int index);
#endif


#endif
#endif