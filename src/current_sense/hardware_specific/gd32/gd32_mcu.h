
#ifndef GD32_CURRENTSENSE_MCU_DEF
#define GD32_CURRENTSENSE_MCU_DEF
#include "../../hardware_api.h"
#include "../../../common/foc_utils.h"

// Only the GD32F1x0 is supported (gd32f130/gd32f130_mcu.cpp). On the other GD32 families, the generic functions
// of SimpleFOC are used: the low side current sense fails at currentSense.init(), and _readRegularADCVoltage
// is not declared.
#if defined(_GD32_DEF_) && defined(GD32F1x0)

// generic implementation of the hardware specific structure
// containing all the necessary current sense parameters
// will be returned as a void pointer from the _configureADCx functions
// will be provided to the _readADCVoltageX() as a void pointer
typedef struct GD32CurrentSenseParams {
  int pins[3] = {(int)NOT_SET};
  float adc_voltage_conv;
} GD32CurrentSenseParams;

// Build flag -D SIMPLEFOC_GD32_ADC_INTERRUPT (as SIMPLEFOC_STM32_ADC_INTERRUPT on STM32): the end of the
// injected conversions raises an interrupt that stores the phase current values, and _readADCVoltageLowSide
// returns the stored values. Without it (default), _readADCVoltageLowSide reads the ADC data registers.

/**
 * Read a regular ADC channel (bus voltage, temperature, potentiometer...) while the injected channels
 * measure the phase currents, as _readRegularADCVoltage on STM32.
 *
 * 12 bit, blocking for a few conversions, on the ADC of the low side current sense: the injected conversions of
 * the currents have priority and are not changed. Several pins can be read in turn, each read waits for a
 * conversion of its own pin (a few us). Much faster than analogRead, which spends ~0.6 ms in the core's code on the
 * GD32F130.
 * Read all your ADC pins with this function, not with the core's analogRead: analogRead does not wait for the
 * conversion of the new pin and can return the value of the pin read before (right only while its code is slow).
 * Do not read a phase current pin: its injected conversion would become slower (sample time set per channel).
 * The core's analogRead used to return the value of the previously read pin for a few reads while the motor runs
 * (0.02-0.05 %): filter the value or wait for several reads before acting on it (e.g. an under/overvoltage cut).
 *
 * Unlike STM32: make a first read (this function or analogRead, any pin) BEFORE currentSense.init().
 * The core configures and calibrates the ADC at its first read; after the current sense init it would
 * calibrate it again while the currents are measured. (Tried: no visible effect on the currents, motor running.)
 *
 * ArduinoCore-GD32 bug, also with analogRead: at this first read, the core writes a wrong alternate function
 * on the pins that follow the read pin in the same AFSEL register (e.g. PA5-PA7 for PA4), because GD_PIN_AF_GET
 * also returns the ADC channel. No effect if those pins are not used as alternate function (timer, SPI, UART...).
 *
 * @param pin - the Arduino pin to read (ADC pin)
 * @return float - the voltage read on the pin, or -1.0f if the pin is not an ADC pin or the conversion times out
 */
float _readRegularADCVoltage(const int pin);

#endif
#endif