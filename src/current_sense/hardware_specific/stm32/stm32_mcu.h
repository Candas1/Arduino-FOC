
#ifndef STM32_CURRENTSENSE_MCU_DEF
#define STM32_CURRENTSENSE_MCU_DEF
#include "Arduino.h"
#include "../../hardware_api.h"
#include "../../../common/foc_utils.h"
#include "../../../drivers/hardware_api.h"
#include "../../../drivers/hardware_specific/stm32/stm32_mcu.h"
#include "../../../communication/SimpleFOCDebug.h"
#include "stm32_utils.h"
#include "stm32_adc.h"
#include "stm32_opamp.h"

#if defined(_STM32_DEF_)

#if defined(STM32F1xx) || defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32G4xx) || defined(STM32L4xx)
#define STM_CURRENT_SENSE_SUPPORTED
#endif

enum current_sense_type : uint8_t {
  INLINE = 0,
  LOWSIDE = 1
};

// generic implementation of the hardware specific structure
// containing all the necessary current sense parameters
// will be returned as a void pointer from the _configureADCx functions
// will be provided to the _readADCVoltageX() as a void pointer
typedef struct Stm32CurrentSenseParams {
  int pins[3] = {(int)NOT_SET};
  float adc_voltage_conv;
  int samples[3] = {NP,NP,NP};
  uint32_t inj_trigger = NP;
  uint32_t reg_trigger = NP;
  HardwareTimer* timer_handle = NP;
  current_sense_type type;
  uint8_t use_adc_interrupt = NP;
} Stm32CurrentSenseParams;

int _adc_init(Stm32CurrentSenseParams* cs_params, const STM32DriverParams* driver_params);
void _adc_gpio_init(Stm32CurrentSenseParams* cs_params, const int pinA, const int pinB, const int pinC);

#endif
#endif