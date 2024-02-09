#ifndef STM32_DRIVER_MCU_DEF
#define STM32_DRIVER_MCU_DEF
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

// default pwm parameters
#define _PWM_RESOLUTION 12 // 12bit
#define _PWM_RANGE 4095.0f // 2^12 -1 = 4095
#define _PWM_FREQUENCY 25000 // 25khz
#define _PWM_FREQUENCY_MAX 50000 // 50khz

// 6pwm parameters
#define _HARDWARE_6PWM 1
#define _SOFTWARE_6PWM 0
#define _ERROR_6PWM -1

// To be use as sampling point on T1 and T8
#ifndef SIMPLEFOC_STM32_SAMPLING_POINT
  #define SIMPLEFOC_STM32_SAMPLING_POINT 99
#endif

typedef struct STM32DriverParams {
  HardwareTimer* timers[6] = {NULL};
  uint32_t channels[6];
  long pwm_frequency;
  float dead_zone;
  uint8_t interface_type;
  uint8_t use_CC4 = 0;
} STM32DriverParams;

// timer synchornisation functions
void _stopTimers(HardwareTimer **timers_to_stop, int timer_num=6);
void _startTimers(HardwareTimer **timers_to_start, int timer_num=6);

#endif
#endif