#include "stm32_utils.h"

#if defined(_STM32_DEF_)

// timer to injected TRGO
// https://github.com/stm32duino/Arduino_Core_STM32/blob/e156c32db24d69cb4818208ccc28894e2f427cfa/system/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_adc_ex.h#L179
uint32_t _timerToInjectedTRGO(HardwareTimer* timer){
#if defined(STM32F1xx)
  if(timer->getHandle()->Instance == TIM1)  
    return ADC_EXTERNALTRIGINJECCONV_T1_TRGO;
#ifdef TIM2 // if defined timer 2
  else if(timer->getHandle()->Instance == TIM2) 
    return ADC_EXTERNALTRIGINJECCONV_T2_TRGO;
#endif
#ifdef TIM4 // if defined timer 4
  else if(timer->getHandle()->Instance == TIM4) 
    return ADC_EXTERNALTRIGINJECCONV_T4_TRGO;
#endif
#ifdef TIM5 // if defined timer 5
  else if(timer->getHandle()->Instance == TIM5) 
    return ADC_EXTERNALTRIGINJECCONV_T5_TRGO;
#endif
#endif
#if defined(STM32F4xx)
  if(timer->getHandle()->Instance == TIM1)  
    return ADC_EXTERNALTRIGINJECCONV_T1_TRGO;
#ifdef TIM2 // if defined timer 2
  else if(timer->getHandle()->Instance == TIM2) 
    return ADC_EXTERNALTRIGINJECCONV_T2_TRGO;
#endif
#ifdef TIM4 // if defined timer 4
  else if(timer->getHandle()->Instance == TIM4) 
    return ADC_EXTERNALTRIGINJECCONV_T4_TRGO;
#endif
#ifdef TIM5 // if defined timer 5
  else if(timer->getHandle()->Instance == TIM5) 
    return ADC_EXTERNALTRIGINJECCONV_T5_TRGO;
#endif
#endif
#if defined(STM32G4xx)
if(timer->getHandle()->Instance == TIM1)  
    return ADC_EXTERNALTRIGINJEC_T1_TRGO;
#ifdef TIM2 // if defined timer 2
  else if(timer->getHandle()->Instance == TIM2) 
    return ADC_EXTERNALTRIGINJEC_T2_TRGO;
#endif
#ifdef TIM3 // if defined timer 3
  else if(timer->getHandle()->Instance == TIM3) 
    return ADC_EXTERNALTRIGINJEC_T3_TRGO;
#endif
#ifdef TIM4 // if defined timer 4
  else if(timer->getHandle()->Instance == TIM4) 
    return ADC_EXTERNALTRIGINJEC_T4_TRGO;
#endif
#ifdef TIM6 // if defined timer 6
  else if(timer->getHandle()->Instance == TIM6) 
    return ADC_EXTERNALTRIGINJEC_T6_TRGO;
#endif
#ifdef TIM7 // if defined timer 7
  else if(timer->getHandle()->Instance == TIM7) 
    return ADC_EXTERNALTRIGINJEC_T7_TRGO;
#endif
#ifdef TIM8 // if defined timer 8
  else if(timer->getHandle()->Instance == TIM8) 
    return ADC_EXTERNALTRIGINJEC_T8_TRGO;
#endif
#ifdef TIM15 // if defined timer 15
  else if(timer->getHandle()->Instance == TIM15) 
    return ADC_EXTERNALTRIGINJEC_T15_TRGO;
#endif
#ifdef TIM20 // if defined timer 15
  else if(timer->getHandle()->Instance == TIM20) 
    return ADC_EXTERNALTRIGINJEC_T20_TRGO;
#endif
#endif
#if defined(STM32L4xx)
if(timer->getHandle()->Instance == TIM1)  
    return ADC_EXTERNALTRIGINJEC_T1_TRGO;
#ifdef TIM2 // if defined timer 2
  else if(timer->getHandle()->Instance == TIM2) 
    return ADC_EXTERNALTRIGINJEC_T2_TRGO;
#endif
#ifdef TIM3 // if defined timer 3
  else if(timer->getHandle()->Instance == TIM3) 
    return ADC_EXTERNALTRIGINJEC_T3_TRGO;
#endif
#ifdef TIM4 // if defined timer 4
  else if(timer->getHandle()->Instance == TIM4) 
    return ADC_EXTERNALTRIGINJEC_T4_TRGO;
#endif
#ifdef TIM6 // if defined timer 6
  else if(timer->getHandle()->Instance == TIM6) 
    return ADC_EXTERNALTRIGINJEC_T6_TRGO;
#endif
#ifdef TIM8 // if defined timer 8
  else if(timer->getHandle()->Instance == TIM8) 
    return ADC_EXTERNALTRIGINJEC_T8_TRGO;
#endif
#ifdef TIM15 // if defined timer 15
  else if(timer->getHandle()->Instance == TIM15) 
    return ADC_EXTERNALTRIGINJEC_T15_TRGO;
#endif
#endif
  else
    return _TRGO_NOT_AVAILABLE;
}


int _adcToIndex(ADC_TypeDef *AdcHandle){
  if(AdcHandle == ADC1) return 0;
#ifdef ADC2 // if ADC2 exists
  else if(AdcHandle == ADC2) return 1;
#endif
#ifdef ADC3 // if ADC3 exists
  else if(AdcHandle == ADC3) return 2;
#endif
#ifdef ADC4 // if ADC4 exists
  else if(AdcHandle == ADC4) return 3;
#endif
#ifdef ADC5 // if ADC5 exists
  else if(AdcHandle == ADC5) return 4;
#endif
  return 0;
}
int _adcToIndex(ADC_HandleTypeDef *AdcHandle){
  return _adcToIndex(AdcHandle->Instance);
}

#endif