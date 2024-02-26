
#ifndef STM32_ADC_DEF
#define STM32_ADC_DEF
#include "Arduino.h"
#include "../../hardware_api.h"
#include "../../../common/foc_utils.h"
#include "../../../drivers/hardware_api.h"
#include "../../../drivers/hardware_specific/stm32/stm32_mcu.h"
#include "stm32_utils.h"


#if defined(_STM32_DEF_) 

#ifdef ADC1
#define ADC1_COUNT 1
#else 
#define ADC1_COUNT 0 
#endif
#ifdef ADC2
#define ADC2_COUNT 1
#else 
#define ADC2_COUNT 0 
#endif
#ifdef ADC3
#define ADC3_COUNT 1
#else 
#define ADC3_COUNT 0 
#endif
#ifdef ADC4
#define ADC4_COUNT 1
#else 
#define ADC4_COUNT 0 
#endif
#ifdef ADC5
#define ADC5_COUNT 1
#else 
#define ADC5_COUNT 0 
#endif

#define ADC_COUNT (ADC1_COUNT + ADC2_COUNT + ADC3_COUNT + ADC4_COUNT + ADC5_COUNT)

typedef struct Stm32ADCSample {
  ADC_TypeDef* Instance = NP;
  ADC_HandleTypeDef* handle = NP;
  int type = NP;
  int pin = NP;
  uint32_t rank = NP;
  uint32_t channel = NP;
  uint adc_index = NP;
  int index = NP;
  uint32_t trigger = NP;
  uint32_t SamplingTime = NP;
} Stm32ADCSample;

int _add_ADC_pin(uint32_t pin,int32_t trigger,int type);
int _init_ADCs();
int _adc_init(Stm32ADCSample sample);
int _adc_inj_channel_config(Stm32ADCSample sample);
int _adc_reg_channel_config(Stm32ADCSample sample);
ADC_HandleTypeDef *_adc_get_handle(ADC_TypeDef* Instance);
ADC_InjectionConfTypeDef *_adc_get_config(ADC_TypeDef* Instance);
int _calibrate_ADC(ADC_HandleTypeDef* hadc);
int _start_ADC(ADC_HandleTypeDef* hadc);
int _start_ADC_IT(ADC_HandleTypeDef* hadc);
int _start_ADCs(void);
int _start_DMA(ADC_HandleTypeDef* hadc);
void _read_ADC(ADC_HandleTypeDef* hadc);
void _read_ADCs(void);
uint32_t _read_adc_buf(int adc_index,int index);
uint32_t _read_adc_dma(int adc_index,int index);
uint32_t _read_adc_sample(int index);
uint32_t _read_adc_pin(int pin);
uint32_t _getADCChannel(PinName pin);
uint32_t _getInjADCRank(int index);
uint32_t _getRegADCRank(int index);
uint32_t _getDMARequest(int index);
DMA_Channel_TypeDef *_getDMAChannel(int index);

#ifdef ARDUINO_B_G431B_ESC1
void _configureOPAMP(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *OPAMPx_Def);
#endif
void _configureOPAMPs(void);
void MX_DMA1_Init(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma_adc, DMA_Channel_TypeDef channel, uint32_t request);

#endif
#endif