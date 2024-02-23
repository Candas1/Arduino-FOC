
#ifndef STM32_CURRENTSENSE_MCU_DEF
#define STM32_CURRENTSENSE_MCU_DEF
#include "../../hardware_api.h"
#include "../../../common/foc_utils.h"
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
int _adc_init(Stm32CurrentSenseParams* cs_params, ADC_TypeDef *Instance,ADC_HandleTypeDef &hadc, ADC_InjectionConfTypeDef &sConfigInjected, int index);
void _adc_gpio_init(Stm32CurrentSenseParams* cs_params, const int pinA, const int pinB, const int pinC);

#endif
#endif