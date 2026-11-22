#ifndef GD32_DRIVER_MCU_DEF
#define GD32_DRIVER_MCU_DEF
#include "../../hardware_api.h"

#if defined(_GD32_DEF_)   // Declared in ArduinoCore-GD32/cores/arduino/gd32/gd32_def.h

// default pwm parameters
#define _PWM_RESOLUTION 12 // 12bit
#define _PWM_FREQUENCY 16000 // 16khz
#define _PWM_FREQUENCY_MAX 50000 // 50khz

// 6pwm parameters
#define _HARDWARE_6PWM 1
#define _SOFTWARE_6PWM 0
#define _ERROR_6PWM -1

// 6PWM needs complementary outputs: on the GD32F1x0 only the advanced timer TIMER0 has them.
// The channels and the alternate functions are taken from the pins (PinMap_PWM of the variant).
#define TIMER_BLDC TIMER0
#define RCU_TIMER_BLDC RCU_TIMER0

// Channel for Sampling Point (the channel without complementary output, not used by the phases)
#define TIMER_BLDC_CHANNEL_S TIMER_CH_3
#define SAMPLING_POINT 0.98 // The closer to 1 the closer to the middle of LOW side ON time. Don't use 1, it will overlap with the update event and not trigger

// Break input (TIMER0_BRKIN, alternate function 2), active low.
// Can be changed with a build flag, or disabled with -D TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN=NOT_SET
#ifndef TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN
#define TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN PB12
#endif

typedef struct GD32DriverParams {
  uint32_t timers[6] = {NULL};
  uint32_t channels[6];
  long pwm_frequency;
  long unsigned int range;
  float dead_zone;
  uint8_t interface_type;
} GD32DriverParams;


#endif
#endif