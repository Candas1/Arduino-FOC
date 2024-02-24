
#ifndef STM32_CURRENTSENSE_MCU_DEF
#define STM32_CURRENTSENSE_MCU_DEF
#include "Arduino.h"
#include "../../hardware_api.h"
#include "../../../common/foc_utils.h"
#include "../../../drivers/hardware_api.h"
#include "../../../drivers/hardware_specific/stm32/stm32_mcu.h"
#include "stm32_utils.h"


#if defined(_STM32_DEF_) 

// generic implementation of the hardware specific structure
// containing all the necessary current sense parameters
// will be returned as a void pointer from the _configureADCx functions
// will be provided to the _readADCVoltageX() as a void pointer
typedef struct Stm32CurrentSenseParams {
  int pins[3] = {(int)NOT_SET};
  float adc_voltage_conv;
  ADC_HandleTypeDef* adc_handle[3] = {NP,NP,NP};
  uint32_t adc_rank[3] = {NP,NP,NP};
  uint adc_index[3] = {NP,NP,NP};
  uint32_t trigger_flag;
  HardwareTimer* timer_handle = NP;
} Stm32CurrentSenseParams;

int _adc_init(Stm32CurrentSenseParams* cs_params, const STM32DriverParams* driver_params);
int _adc_init(Stm32CurrentSenseParams* cs_params, ADC_TypeDef *Instance,ADC_HandleTypeDef* hadc);
int _adc_channel_config(Stm32CurrentSenseParams* cs_params, ADC_TypeDef *Instance,ADC_HandleTypeDef* hadc, ADC_InjectionConfTypeDef* sConfigInjected, int index);
void _adc_gpio_init(Stm32CurrentSenseParams* cs_params, const int pinA, const int pinB, const int pinC);
ADC_HandleTypeDef *_adc_get_handle(ADC_TypeDef* Instance);
ADC_InjectionConfTypeDef *_adc_get_config(ADC_TypeDef* Instance);
int _calibrate_ADC(ADC_HandleTypeDef* hadc);
int _start_ADC(ADC_HandleTypeDef* hadc);
int _start_ADC_IT(ADC_HandleTypeDef* hadc);
void _start_ADCs(void);
void _read_ADC(ADC_HandleTypeDef* hadc);
void _read_ADCs(void);

#ifdef ARDUINO_B_G431B_ESC1
void _configureOPAMP(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *OPAMPx_Def);
#endif

#endif
#endif