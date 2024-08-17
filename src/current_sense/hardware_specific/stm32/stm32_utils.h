
#ifndef STM32_UTILS_HAL
#define STM32_UTILS_HAL

#include "Arduino.h"

#if defined(_STM32_DEF_)

#define _TRGO_NOT_AVAILABLE 12345

// Only if injected ADC available
#ifdef ADC_INJECTED_SOFTWARE_START
uint32_t _timerToInjectedTRGO(HardwareTimer* timer); // timer to injected TRGO
uint32_t _getInjADCRank(int index);
#endif

uint32_t _timerToRegularTRGO(HardwareTimer* timer); // timer to regular TRGO

// function returning index of the ADC instance
int _adcToIndex(ADC_HandleTypeDef *AdcHandle);
int _adcToIndex(ADC_TypeDef *AdcInstance);
uint32_t _getADCChannel(PinName pin);
uint32_t _getRegADCRank(int index);
ADC_HandleTypeDef *_get_ADC_handle(ADC_TypeDef* Instance);

#endif

#endif