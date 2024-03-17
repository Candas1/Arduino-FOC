
#ifndef STM32_UTILS_HAL
#define STM32_UTILS_HAL

#include "Arduino.h"

#if defined(_STM32_DEF_)

#define _TRGO_NOT_AVAILABLE 12345

// timer to injected TRGO
// https://github.com/stm32duino/Arduino_Core_STM32/blob/e156c32db24d69cb4818208ccc28894e2f427cfa/system/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_adc_ex.h#L179
uint32_t _timerToInjectedTRGO(HardwareTimer* timer);

// timer to regular TRGO
// https://github.com/stm32duino/Arduino_Core_STM32/blob/e156c32db24d69cb4818208ccc28894e2f427cfa/system/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_adc.h#L331
uint32_t _timerToRegularTRGO(HardwareTimer* timer);

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
#if defined(STM32F4xx)
DMA_Stream_TypeDef *_getDMAStream(int index);
#endif
ADC_HandleTypeDef *_get_ADC_handle(ADC_TypeDef* Instance);
uint32_t _is_enabled_ADC(ADC_HandleTypeDef* hadc);

#endif

#endif