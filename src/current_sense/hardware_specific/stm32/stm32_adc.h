
#ifndef STM32_ADC_DEF
#define STM32_ADC_DEF
#include "Arduino.h"
#include "../../hardware_api.h"
#include "../../../common/foc_utils.h"
#include "../../../drivers/hardware_api.h"
#include "../../../drivers/hardware_specific/stm32/stm32_mcu.h"
#include "../../../communication/SimpleFOCDebug.h"
#include "stm32_utils.h"
#include "stm32_dma.h"
#include "stm32_mcu.h"


#if defined(_STM32_DEF_) 

#if defined(ADC1) 
#define ADC1_COUNT 1
#else 
#define ADC1_COUNT 0 
#endif
#if defined(ADC2)
#define ADC2_COUNT 1
#else 
#define ADC2_COUNT 0 
#endif
#if defined(ADC3)
#define ADC3_COUNT 1
#else 
#define ADC3_COUNT 0 
#endif
#if defined(ADC4)
#define ADC4_COUNT 1
#else 
#define ADC4_COUNT 0 
#endif
#if defined(ADC5)
#define ADC5_COUNT 1
#else 
#define ADC5_COUNT 0 
#endif

#define ADC_COUNT (ADC1_COUNT + ADC2_COUNT + ADC3_COUNT + ADC4_COUNT + ADC5_COUNT)

#ifndef MAX_REG_ADC_CHANNELS
  #if !defined(ADC_REGULAR_RANK_1) || defined(ADC_REGULAR_RANK_9)
    #define MAX_REG_ADC_CHANNELS 16 // Maximum number of samples for Regular ADC
  #else
    #define MAX_REG_ADC_CHANNELS 8 // Maximum number of samples for Regular ADC
  #endif
#endif
#define MAX_INJ_ADC_CHANNELS 4  // Maximum number of samples for Injected ADC

#ifndef ADC_SAMPLINGTIME_INJ
#if defined(ADC_SAMPLETIME_1CYCLE_5)
#define ADC_SAMPLINGTIME_INJ ADC_SAMPLETIME_1CYCLE_5;
#elif defined(ADC_SAMPLETIME_2CYCLES_5)
#define ADC_SAMPLINGTIME_INJ ADC_SAMPLETIME_2CYCLES_5;
#elif defined(ADC_SAMPLETIME_3CYCLES)
#define ADC_SAMPLINGTIME_INJ ADC_SAMPLETIME_3CYCLES;
#elif defined(ADC_SAMPLETIME_4CYCLES)
#define ADC_SAMPLINGTIME_INJ ADC_SAMPLETIME_4CYCLES;
#elif defined(ADC_SAMPLETIME_5CYCLES)
#define ADC_SAMPLINGTIME_INJ ADC_SAMPLETIME_5CYCLES;
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
#if defined(ADC_CLOCK_SYNC_PCLK_DIV4)
#define ADC_CLOCK_DIV       ADC_CLOCK_SYNC_PCLK_DIV4
#elif defined(ADC_CLOCK_SYNC_PCLK_DIV2)
#define ADC_CLOCK_DIV       ADC_CLOCK_SYNC_PCLK_DIV2
#elif defined(ADC_CLOCK_ASYNC_DIV4)
#define ADC_CLOCK_DIV       ADC_CLOCK_ASYNC_DIV4
#endif
#endif /* !ADC_CLOCK_DIV */

enum adc_type : uint8_t {
  INJECTED_ADC = 0,
  REGULAR_ADC = 1
};

typedef struct Stm32ADC {
  ADC_TypeDef* Instance = NP;
  ADC_HandleTypeDef* handle = NP;
  int adc_index = NP;
  uint16_t adc_dma_buf[MAX_REG_ADC_CHANNELS]={0}; // Regular ADC buffer
} Stm32ADC;

typedef struct Stm32ADCSample {
  ADC_TypeDef* Instance = NP; // ADC instance
  ADC_HandleTypeDef* handle = NP; // ADC handle
  adc_type type; // 0:injected adc 1:regular adc 
  int pin = NP; // pin
  int adc_index = NP; // ADC index
  uint32_t rank = NP; // Rank for the used adc
  int index = NP; // Rank -1
  uint32_t channel = NP; // Adc channel
  uint32_t SamplingTime = NP; 
  uint8_t buffered = NP; 
  uint16_t buffer = NP;
  void* cs_params = nullptr; // Pointer to current sense params
} Stm32ADCSample;

typedef struct Stm32ADCEngine {
  ADC_HandleTypeDef* adc_handles[ADC_COUNT] = {NP}; // Handles of the ADC that are initialized
  int inj_channel_max = 0; 
  int reg_channel_max = 0;
  Stm32ADCSample samples[ADC_COUNT * (MAX_REG_ADC_CHANNELS+MAX_INJ_ADC_CHANNELS)] = {}; // The maximum number of sample is the number of ADC * 4 injected + 16 regular
  int sample_count = 0;
  ADC_HandleTypeDef *interrupt_adc = NP; // ADC triggering the interrupt
  int inj_channel_count[ADC_COUNT] = {0}; // Total count of injected channels for an ADC 
  int reg_channel_count[ADC_COUNT] = {0}; // Total Count of regular channels for an ADC
  int inj_trigger[ADC_COUNT] = {0}; // Injected Trigger for each ADC
  int reg_trigger[ADC_COUNT] = {0}; // Regular Trigger for each ADC
} Stm32ADCEngine;

int _add_ADC_sample(uint32_t pin,int32_t trigger,adc_type type, void* _csparams = nullptr);
int _init_ADCs();
int _init_ADC(Stm32ADCSample sample);
int _add_reg_ADC_channel_config(Stm32ADCSample sample);
int _calibrate_ADC(ADC_HandleTypeDef* hadc);
int _start_ADCs(void* _csparams);
uint32_t _read_ADC_sample(int index);
uint32_t _read_ADC_register(int index);
uint32_t _read_ADC_pin(int pin);

#ifdef ADC_INJECTED_SOFTWARE_START
int _add_inj_ADC_channel_config(Stm32ADCSample sample);
int _start_inj_ADC(ADC_HandleTypeDef* hadc);
int _start_inj_ADC_IT(ADC_HandleTypeDef* hadc);
#endif
int _start_reg_ADC(ADC_HandleTypeDef* hadc);
int _start_reg_ADC_IT(ADC_HandleTypeDef* hadc);

void _enable_irq(ADC_HandleTypeDef* hadc);
void _buffer_ADCs(void);

#endif
#endif