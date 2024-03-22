
#ifndef STM32_UTILS_HAL
#define STM32_UTILS_HAL

#include "Arduino.h"

#if defined(_STM32_DEF_)

#define _TRGO_NOT_AVAILABLE 12345

uint32_t _timerToInjectedTRGO(HardwareTimer* timer); // timer to injected TRGO
uint32_t _timerToRegularTRGO(HardwareTimer* timer); // timer to regular TRGO

// function returning index of the ADC instance
int _adcToIndex(ADC_HandleTypeDef *AdcHandle);
int _adcToIndex(ADC_TypeDef *AdcHandle);
uint32_t _getADCChannel(PinName pin);
uint32_t _getInjADCRank(int index);
uint32_t _getRegADCRank(int index);
uint32_t _getDMARequest(int index);
#if defined(STM32F2xx) || defined(STM32F4xx) || defined(STM32F7xx)
uint32_t _getDMAChannel(int index);
#else
DMA_Channel_TypeDef *_getDMAChannel(int index);
#endif
#if defined(STM32F4xx) || defined(STM32F7xx)
DMA_Stream_TypeDef *_getDMAStream(int index);
#endif
ADC_HandleTypeDef *_get_ADC_handle(ADC_TypeDef* Instance);

#endif

#endif