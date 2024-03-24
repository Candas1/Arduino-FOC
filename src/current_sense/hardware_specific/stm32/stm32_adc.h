
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


#ifndef ADC_SAMPLINGTIME_INJ
#if defined(ADC_SAMPLETIME_1CYCLE_5)
#define ADC_SAMPLINGTIME_INJ ADC_SAMPLETIME_1CYCLE_5;
#elif defined(ADC_SAMPLETIME_2CYCLES_5)
#define ADC_SAMPLINGTIME_INJ ADC_SAMPLETIME_2CYCLES_5;
#elif defined(ADC_SAMPLETIME_3CYCLES)
#define ADC_SAMPLINGTIME_INJ ADC_SAMPLETIME_3CYCLES;
#endif
#endif /* !ADC_SAMPLINGTIME */

#ifndef ADC_SAMPLINGTIME
#if defined(ADC_SAMPLETIME_8CYCLES_5)
#define ADC_SAMPLINGTIME        ADC_SAMPLETIME_8CYCLES_5;
#elif defined(ADC_SAMPLETIME_12CYCLES)
#define ADC_SAMPLINGTIME        ADC_SAMPLETIME_12CYCLES;
#elif defined(ADC_SAMPLETIME_12CYCLES_5)
#define ADC_SAMPLINGTIME        ADC_SAMPLETIME_12CYCLES_5;
#elif defined(ADC_SAMPLETIME_13CYCLES_5)
#define ADC_SAMPLINGTIME        ADC_SAMPLETIME_13CYCLES_5;
#elif defined(ADC_SAMPLETIME_15CYCLES)
#define ADC_SAMPLINGTIME        ADC_SAMPLETIME_15CYCLES;
#elif defined(ADC_SAMPLETIME_16CYCLES)
#define ADC_SAMPLINGTIME        ADC_SAMPLETIME_16CYCLES;
#elif defined(ADC_SAMPLETIME_19CYCLES_5)
#define ADC_SAMPLINGTIME        ADC_SAMPLETIME_19CYCLES_5;
#endif
#endif /* !ADC_SAMPLINGTIME */

/*
 * Minimum ADC sampling time is required when reading
 * internal channels so set it to max possible value.
 * It can be defined more precisely by defining:
 * ADC_SAMPLINGTIME_INTERNAL
 * to the desired ADC sample time.
 */
#ifndef ADC_SAMPLINGTIME_INTERNAL
#if defined(ADC_SAMPLETIME_480CYCLES)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_480CYCLES
#elif defined(ADC_SAMPLETIME_384CYCLES)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_384CYCLES
#elif defined(ADC_SAMPLETIME_810CYCLES_5)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_810CYCLES_5
#elif defined(ADC_SAMPLETIME_814CYCLES)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_814CYCLES
#elif defined(ADC_SAMPLETIME_640CYCLES_5)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_640CYCLES_5
#elif defined(ADC_SAMPLETIME_601CYCLES_5)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_601CYCLES_5
#elif defined(ADC_SAMPLETIME_247CYCLES_5)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_247CYCLES_5
#elif defined(ADC_SAMPLETIME_239CYCLES_5)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_239CYCLES_5
#elif defined(ADC_SAMPLETIME_160CYCLES_5)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_160CYCLES_5
#elif defined(ADC_SAMPLETIME_814CYCLES_5)
#define ADC_SAMPLINGTIME_INTERNAL ADC_SAMPLETIME_814CYCLES_5
#else
#error "ADC sampling time could not be defined for internal channels!"
#endif
#endif /* !ADC_SAMPLINGTIME_INTERNAL */

#ifndef ADC_CLOCK_DIV
#ifdef ADC_CLOCK_SYNC_PCLK_DIV4
#define ADC_CLOCK_DIV       ADC_CLOCK_SYNC_PCLK_DIV4
#elif ADC_CLOCK_SYNC_PCLK_DIV2
#define ADC_CLOCK_DIV       ADC_CLOCK_SYNC_PCLK_DIV2
#elif defined(ADC_CLOCK_ASYNC_DIV4)
#define ADC_CLOCK_DIV       ADC_CLOCK_ASYNC_DIV4
#endif
#endif /* !ADC_CLOCK_DIV */

typedef struct Stm32ADCSample {
  ADC_TypeDef* Instance = NP;
  ADC_HandleTypeDef* handle = NP;
  int type = NP;
  int pin = NP;
  uint32_t rank = NP;
  uint32_t channel = NP;
  int adc_index = NP;
  int index = NP;
  uint32_t SamplingTime = NP;
} Stm32ADCSample;


int _add_inj_ADC_sample(uint32_t pin,int32_t trigger);
int _add_reg_ADC_sample(uint32_t pin);
int _add_ADC_sample(uint32_t pin,int32_t trigger,int type);
int _init_ADCs();
int _init_ADC(Stm32ADCSample sample);
int _init_DMA(ADC_HandleTypeDef *hadc);
#ifdef ARDUINO_B_G431B_ESC1
int _init_OPAMP(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *OPAMPx_Def);
int _init_OPAMPs(void);
#endif
int _add_reg_ADC_channel_config(Stm32ADCSample sample);
int _calibrate_ADC(ADC_HandleTypeDef* hadc);
int _start_ADCs(int use_adc_interrupt = 0);
int _start_DMA(ADC_HandleTypeDef* hadc);
uint32_t _read_ADC_buf(int adc_index,int index);
uint32_t _read_DMA_buf(int adc_index,int index);
uint32_t _read_ADC_sample(int index);
uint32_t _read_ADC_pin(int pin);

#ifdef ADC_INJECTED_SOFTWARE_START
int _add_inj_ADC_channel_config(Stm32ADCSample sample);
int _start_inj_ADC(ADC_HandleTypeDef* hadc);
int _start_inj_ADC_IT(ADC_HandleTypeDef* hadc);
void _read_inj_ADC(ADC_HandleTypeDef* hadc);
void _read_inj_ADCs(void);
#endif


#endif
#endif