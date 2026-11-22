
#ifndef GD32_CURRENTSENSE_MCU_DEF
#define GD32_CURRENTSENSE_MCU_DEF
#include "../../hardware_api.h"
#include "../../../common/foc_utils.h"

#if defined(_GD32_DEF_) 

// generic implementation of the hardware specific structure
// containing all the necessary current sense parameters
// will be returned as a void pointer from the _configureADCx functions
// will be provided to the _readADCVoltageX() as a void pointer
typedef struct GD32CurrentSenseParams {
  int pins[3] = {(int)NOT_SET};
  float adc_voltage_conv;
  //ADC_HandleTypeDef* adc_handle = NP;
  //HardwareTimer* timer_handle = NP;
} GD32CurrentSenseParams;

//#define ADC_USE_INTERRUPT         // If defined, will use the End of conversion interrupt of inserted ADC
#define PHASE_CURRENT_FILTER 0.10 // Used with the interrupt. If 1.00, no filter

/**
 * Read a regular ADC channel (bus voltage, temperature, potentiometer...) while the injected channels
 * measure the phase currents, as _readRegularADCVoltage on STM32.
 *
 * One-shot regular conversion started by software on the ADC of the low side current sense. Injected
 * conversions have hardware priority: the current sensing is not changed, the reading can wait for them.
 * The ADC is initialised if the current sense has not done it yet.
 * Slower than the injected reading (a few us, blocking) but does not reconfigure the ADC like analogRead.
 *
 * @param pin - the Arduino pin to read (ADC pin)
 * @return float - the voltage read on the pin, or -1.0f on error (not an ADC pin, timeout)
 */
float _readRegularADCVoltage(const int pin);

#endif
#endif