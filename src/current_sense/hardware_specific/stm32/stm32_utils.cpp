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
  
  return _TRGO_NOT_AVAILABLE;
}

// timer to regular TRGO
// https://github.com/stm32duino/Arduino_Core_STM32/blob/6588dee03382e73ed42c4a5e473900ab3b79d6e4/system/Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_hal_adc.h#L519
uint32_t _timerToRegularTRGO(HardwareTimer* timer){
#if defined(STM32F1xx)
  if(timer->getHandle()->Instance == TIM3) 
    return ADC_EXTERNALTRIGCONV_T3_TRGO;
#ifdef TIM8 // if defined timer 8
  else if(timer->getHandle()->Instance == TIM8) 
    return ADC_EXTERNALTRIGCONV_T8_TRGO;
#endif
#endif

#if defined(STM32F4xx)
  if(timer->getHandle()->Instance == TIM2)  
    return ADC_EXTERNALTRIGCONV_T2_TRGO;
#ifdef TIM3 // if defined timer 3
  else if(timer->getHandle()->Instance == TIM3) 
    return ADC_EXTERNALTRIGCONV_T3_TRGO;
#endif
#ifdef TIM8 // if defined timer 8
  else if(timer->getHandle()->Instance == TIM8) 
    return ADC_EXTERNALTRIGCONV_T8_TRGO;
#endif
#endif

#if defined(STM32G4xx)
  if(timer->getHandle()->Instance == TIM1)  
    return ADC_EXTERNALTRIG_T1_TRGO;
#ifdef TIM2 // if defined timer 2
  else if(timer->getHandle()->Instance == TIM2) 
    return ADC_EXTERNALTRIG_T2_TRGO;
#endif
#ifdef TIM3 // if defined timer 3
  else if(timer->getHandle()->Instance == TIM3) 
    return ADC_EXTERNALTRIG_T3_TRGO;
#endif
#ifdef TIM4 // if defined timer 4
  else if(timer->getHandle()->Instance == TIM4) 
    return ADC_EXTERNALTRIG_T4_TRGO;
#endif
#ifdef TIM6 // if defined timer 6
  else if(timer->getHandle()->Instance == TIM6) 
    return ADC_EXTERNALTRIG_T6_TRGO;
#endif
#ifdef TIM7 // if defined timer 7
  else if(timer->getHandle()->Instance == TIM7) 
    return ADC_EXTERNALTRIG_T7_TRGO;
#endif
#ifdef TIM8 // if defined timer 8
  else if(timer->getHandle()->Instance == TIM8) 
    return ADC_EXTERNALTRIG_T7_TRGO;
#endif
#ifdef TIM15 // if defined timer 15
  else if(timer->getHandle()->Instance == TIM15) 
    return ADC_EXTERNALTRIG_T15_TRGO;
#endif
#ifdef TIM20 // if defined timer 15
  else if(timer->getHandle()->Instance == TIM20) 
    return ADC_EXTERNALTRIG_T20_TRGO;
#endif
#endif

#if defined(STM32L4xx)
  if(timer->getHandle()->Instance == TIM1)  
    return ADC_EXTERNALTRIG_T1_TRGO;
#ifdef TIM2 // if defined timer 2
  else if(timer->getHandle()->Instance == TIM2) 
    return ADC_EXTERNALTRIG_T2_TRGO;
#endif
#ifdef TIM3 // if defined timer 3
  else if(timer->getHandle()->Instance == TIM3) 
    return ADC_EXTERNALTRIG_T3_TRGO;
#endif
#ifdef TIM4 // if defined timer 4
  else if(timer->getHandle()->Instance == TIM4) 
    return ADC_EXTERNALTRIG_T4_TRGO;
#endif
#ifdef TIM6 // if defined timer 6
  else if(timer->getHandle()->Instance == TIM6) 
    return ADC_EXTERNALTRIG_T6_TRGO;
#endif
#ifdef TIM8 // if defined timer 8
  else if(timer->getHandle()->Instance == TIM8) 
    return ADC_EXTERNALTRIG_T8_TRGO;
#endif
#ifdef TIM15 // if defined timer 15
  else if(timer->getHandle()->Instance == TIM15) 
    return ADC_EXTERNALTRIG_T15_TRGO;
#endif
#endif

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

uint32_t _getDMARequest(int index){
  switch(index){
    #if defined(STM32F1xx) || defined(STM32G4xx) || defined(STM32L4xx)
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

#if defined(STM32F4xx)
uint32_t _getDMAChannel(int index){
#else
DMA_Channel_TypeDef *_getDMAChannel(int index){
#endif
  switch(index){
    #if defined(STM32F4xx)
    #ifdef DMA_CHANNEL_0
    case 0:   
      return DMA_CHANNEL_0;
    #endif
    #ifdef DMA_CHANNEL_1
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
    #ifdef DMA_CHANNEL_5
    case 5:
      return DMA_CHANNEL_5;
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

uint32_t _is_enabled_ADC(ADC_HandleTypeDef* hadc){
  #if defined(STM32F1xx) || defined(STM32F4xx)
  return ADC_IS_ENABLE(hadc);
  #endif
  #if defined(STM32G4xx) || defined(STM32L4xx)
  return LL_ADC_IsEnabled(hadc->Instance);
  #endif 
}

#endif