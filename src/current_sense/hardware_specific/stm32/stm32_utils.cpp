#include "stm32_utils.h"

#if defined(_STM32_DEF_)

// timer to injected TRGO
// https://github.com/stm32duino/Arduino_Core_STM32/blob/e156c32db24d69cb4818208ccc28894e2f427cfa/system/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_adc_ex.h#L179
uint32_t _timerToInjectedTRGO(HardwareTimer* timer){

  // This should work with F1 F2 F3 F4 F7
#if defined(ADC_EXTERNALTRIGINJECCONV_T1_TRGO) && defined(TIM1)
  if(timer->getHandle()->Instance == TIM1) return ADC_EXTERNALTRIGINJECCONV_T1_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T2_TRGO) && defined(TIM2)
  if(timer->getHandle()->Instance == TIM2) return ADC_EXTERNALTRIGINJECCONV_T2_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T3_TRGO) && defined(TIM3)
  if(timer->getHandle()->Instance == TIM3) return ADC_EXTERNALTRIGINJECCONV_T3_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T4_TRGO) && defined(TIM4)
  if(timer->getHandle()->Instance == TIM4) return ADC_EXTERNALTRIGINJECCONV_T4_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T5_TRGO) && defined(TIM5)
  if(timer->getHandle()->Instance == TIM5) return ADC_EXTERNALTRIGINJECCONV_T5_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T6_TRGO) && defined(TIM6)
  if(timer->getHandle()->Instance == TIM6) return ADC_EXTERNALTRIGINJECCONV_T6_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T7_TRGO) && defined(TIM7)
  if(timer->getHandle()->Instance == TIM7) return ADC_EXTERNALTRIGINJECCONV_T7_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T8_TRGO) && defined(TIM8)
  if(timer->getHandle()->Instance == TIM8) return ADC_EXTERNALTRIGINJECCONV_T8_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T9_TRGO) && defined(TIM9)
  if(timer->getHandle()->Instance == TIM9) return ADC_EXTERNALTRIGINJECCONV_T9_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T10_TRGO) && defined(TIM10)
  if(timer->getHandle()->Instance == TIM10) return ADC_EXTERNALTRIGINJECCONV_T10_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T11_TRGO) && defined(TIM11)
  if(timer->getHandle()->Instance == TIM11) return ADC_EXTERNALTRIGINJECCONV_T11_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T12_TRGO) && defined(TIM12)
  if(timer->getHandle()->Instance == TIM12) return ADC_EXTERNALTRIGINJECCONV_T12_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T13_TRGO) && defined(TIM13)
  if(timer->getHandle()->Instance == TIM13) return ADC_EXTERNALTRIGINJECCONV_T13_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T14_TRGO) && defined(TIM14)
  if(timer->getHandle()->Instance == TIM14) return ADC_EXTERNALTRIGINJECCONV_T14_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T15_TRGO) && defined(TIM15)
  if(timer->getHandle()->Instance == TIM15) return ADC_EXTERNALTRIGINJECCONV_T15_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T16_TRGO) && defined(TIM16)
  if(timer->getHandle()->Instance == TIM16) return ADC_EXTERNALTRIGINJECCONV_T16_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T17_TRGO) && defined(TIM17)
  if(timer->getHandle()->Instance == TIM17) return ADC_EXTERNALTRIGINJECCONV_T17_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T18_TRGO) && defined(TIM18)
  if(timer->getHandle()->Instance == TIM18) return ADC_EXTERNALTRIGINJECCONV_T18_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T19_TRGO) && defined(TIM19)
  if(timer->getHandle()->Instance == TIM19) return ADC_EXTERNALTRIGINJECCONV_T19_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJECCONV_T20_TRGO) && defined(TIM20)
  if(timer->getHandle()->Instance == TIM20) return ADC_EXTERNALTRIGINJECCONV_T20_TRGO;
#endif

// This should work with G4 H5 H7 L4 L5 MP1 U5 WB
#if defined(ADC_EXTERNALTRIGINJEC_T1_TRGO) && defined(TIM1)
  if(timer->getHandle()->Instance == TIM1) return ADC_EXTERNALTRIGINJEC_T1_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T2_TRGO) && defined(TIM2)
  if(timer->getHandle()->Instance == TIM2) return ADC_EXTERNALTRIGINJEC_T2_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T3_TRGO) && defined(TIM3)
  if(timer->getHandle()->Instance == TIM3) return ADC_EXTERNALTRIGINJEC_T3_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T4_TRGO) && defined(TIM4)
  if(timer->getHandle()->Instance == TIM4) return ADC_EXTERNALTRIGINJEC_T4_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T5_TRGO) && defined(TIM5)
  if(timer->getHandle()->Instance == TIM5) return ADC_EXTERNALTRIGINJEC_T5_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T6_TRGO) && defined(TIM6)
  if(timer->getHandle()->Instance == TIM6) return ADC_EXTERNALTRIGINJEC_T6_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T7_TRGO) && defined(TIM7)
  if(timer->getHandle()->Instance == TIM7) return ADC_EXTERNALTRIGINJEC_T7_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T8_TRGO) && defined(TIM8)
  if(timer->getHandle()->Instance == TIM8) return ADC_EXTERNALTRIGINJEC_T8_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T9_TRGO) && defined(TIM9)
  if(timer->getHandle()->Instance == TIM9) return ADC_EXTERNALTRIGINJEC_T9_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T10_TRGO) && defined(TIM10)
  if(timer->getHandle()->Instance == TIM10) return ADC_EXTERNALTRIGINJEC_T10_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T11_TRGO) && defined(TIM11)
  if(timer->getHandle()->Instance == TIM11) return ADC_EXTERNALTRIGINJEC_T11_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T12_TRGO) && defined(TIM12)
  if(timer->getHandle()->Instance == TIM12) return ADC_EXTERNALTRIGINJEC_T12_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T13_TRGO) && defined(TIM13)
  if(timer->getHandle()->Instance == TIM13) return ADC_EXTERNALTRIGINJEC_T13_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T14_TRGO) && defined(TIM14)
  if(timer->getHandle()->Instance == TIM14) return ADC_EXTERNALTRIGINJEC_T14_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T15_TRGO) && defined(TIM15)
  if(timer->getHandle()->Instance == TIM15) return ADC_EXTERNALTRIGINJEC_T15_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T16_TRGO) && defined(TIM16)
  if(timer->getHandle()->Instance == TIM16) return ADC_EXTERNALTRIGINJEC_T16_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T17_TRGO) && defined(TIM17)
  if(timer->getHandle()->Instance == TIM17) return ADC_EXTERNALTRIGINJEC_T17_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T18_TRGO) && defined(TIM18)
  if(timer->getHandle()->Instance == TIM18) return ADC_EXTERNALTRIGINJEC_T18_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T19_TRGO) && defined(TIM19)
  if(timer->getHandle()->Instance == TIM19) return ADC_EXTERNALTRIGINJEC_T19_TRGO;
#endif

#if defined(ADC_EXTERNALTRIGINJEC_T20_TRGO) && defined(TIM20)
  if(timer->getHandle()->Instance == TIM20) return ADC_EXTERNALTRIGINJEC_T20_TRGO;
#endif

  // Return not available if no trigger was found, not that following families have no injected ADC : C0 F0 G0 L0 WL
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
      return index + 1; // Works for F4
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

#endif