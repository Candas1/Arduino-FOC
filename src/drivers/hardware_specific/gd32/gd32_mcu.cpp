#include "../../hardware_api.h"
#include "gd32_mcu.h"

#if defined(_GD32_DEF_)

// GD32 Arduino core only: must stay inside the guard, the other platforms don't have it
#include "gd32/PinNames.h"
#include "gd32/PeripheralPins.h"

  float sampling_point = SAMPLING_POINT;

// Entry of PinMap_PWM (variant PeripheralPins.c) for this pin on this timer, NULL if there is none.
// A pin can have several timer functions (PB15: TIMER14_CH1 on AF1, TIMER0_CH2_ON on AF2, TIMER14_CH0_ON on AF3):
// the pinmap functions of the core return the first one, so the table is searched for the wanted timer.
const PinMap* _findTimerPin(int pin, uint32_t timer){
  PinName pin_name = DIGITAL_TO_PINNAME(pin);
  for (const PinMap* map = PinMap_PWM; map->pin != NC; map++) {
    if (map->pin == pin_name && (uint32_t)map->peripheral == timer) return map;
  }
  return NULL;
}

// Dead time register value (DTCFG, bits 7:0 of TIMERx_CCHP) for a dead time in timer clock cycles (CKDIV = 1):
//   DTCFG = 0xxxxxxx: DTCFG[6:0] cycles             (0 to 127)
//           10xxxxxx: (64 + DTCFG[5:0]) x 2 cycles  (128 to 254)
//           110xxxxx: (32 + DTCFG[4:0]) x 8 cycles  (256 to 504)
//           111xxxxx: (32 + DTCFG[4:0]) x 16 cycles (512 to 1008)
// Rounded down to the step of the range, as __LL_TIM_CALC_DEADTIME does for STM32. Longer: the maximum (1008 cycles).
uint8_t _deadTimeRegister(uint32_t cycles){
  if (cycles < 128) return cycles;
  if (cycles < 256) return 0x80 | (cycles / 2 - 64);
  if (cycles < 512) return 0xC0 | (cycles / 8 - 32);
  if (cycles < 1024) return 0xE0 | (cycles / 16 - 32);
  return 0xFF;
}

void* _configure6PWM(long pwm_frequency, float dead_zone, const int pinA_h, const int pinA_l, const int pinB_h, const int pinB_l, const int pinC_h, const int pinC_l){
  if( !pwm_frequency || !_isset(pwm_frequency) ) pwm_frequency = _PWM_FREQUENCY; // default frequency 16khz
  else pwm_frequency = _constrain(pwm_frequency, 0, _PWM_FREQUENCY_MAX); // constrain to 50kHz max

  // dead time: dead_zone is a fraction of the PWM period, as for the other platforms (STM32),
  // in cycles of TIMER0, which runs at SystemCoreClock (APB2 not divided), as for the period below
  // (rounded: 0.01f * 72 MHz / 16 kHz gives 44.99999 in float)
  uint32_t dead_time_cycles = _round(dead_zone * SystemCoreClock / pwm_frequency);

  // center-aligned frequency is uses two periods
  pwm_frequency *=2;

  // PWM timer Parameter structs
  timer_parameter_struct timerBldc_parameter_struct;
  timer_break_parameter_struct timerBldc_break_parameter_struct;
  timer_oc_parameter_struct timerBldc_oc_parameter_struct;

  // timer functions of the pins: high side and low side of a phase on the same channel of TIMER0,
  // one channel per phase
  const int pins[6] = { pinA_h, pinA_l, pinB_h, pinB_l, pinC_h, pinC_l };
  const PinMap* maps[6];
  for (int i = 0; i < 6; i++) {
    maps[i] = _findTimerPin(pins[i], TIMER_BLDC);
    if (maps[i] == NULL) {
      SIMPLEFOC_DEBUG("GD32-DRV: ERR: pin not on TIMER0: ", pins[i]);
      return SIMPLEFOC_DRIVER_INIT_FAILED;
    }
  }
  uint16_t channelA = GD_PIN_CHANNEL_GET(maps[0]->function);
  uint16_t channelB = GD_PIN_CHANNEL_GET(maps[2]->function);
  uint16_t channelC = GD_PIN_CHANNEL_GET(maps[4]->function);
  if (GD_PIN_CHANNEL_GET(maps[1]->function) != channelA
      || GD_PIN_CHANNEL_GET(maps[3]->function) != channelB
      || GD_PIN_CHANNEL_GET(maps[5]->function) != channelC
      || channelA == channelB || channelA == channelC || channelB == channelC) {
    SIMPLEFOC_DEBUG("GD32-DRV: ERR: high and low side pins of a phase must share a channel, one channel per phase");
    return SIMPLEFOC_DRIVER_INIT_FAILED;
  }

  // alternate function of each pin from PinMap_PWM, mode and speed as before.
  // The AF index is in bits 15:18 only: GD_PIN_AF_GET masks 8 bits and also returns the channel (bits 19:23),
  // e.g. 34 instead of 2 for CH2, which pin_function would use as an index out of GD_GPIO_AF[] (that wrote a
  // wrong AF on PA13/PA14 and disabled SWD).
  for (int i = 0; i < 6; i++) {
    uint32_t af = (maps[i]->function >> PIN_AF_SHIFT) & 0x0F;
    pin_function(maps[i]->pin, GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, af));
  }

  #ifdef TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN
    pin_function(DIGITAL_TO_PINNAME(TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN), GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, 2));
  #endif

  // Enable timer clock
  rcu_periph_clock_enable(RCU_TIMER_BLDC);

  // Initial deinitialize of the timer
  timer_deinit(TIMER_BLDC);
  //dbg_periph_enable(DBG_TIMER0_HOLD); // Hold counter of timer 0 during debug
 
  // Set up the basic parameter struct for the timer
  timer_struct_para_init(&timerBldc_parameter_struct);
  timerBldc_parameter_struct.counterdirection  = TIMER_COUNTER_UP;
  timerBldc_parameter_struct.prescaler 		     = 0;
  timerBldc_parameter_struct.alignedmode 	     = TIMER_COUNTER_CENTER_DOWN;
  timerBldc_parameter_struct.period			       = SystemCoreClock / pwm_frequency;
  timerBldc_parameter_struct.clockdivision 	   = TIMER_CKDIV_DIV1;
  timerBldc_parameter_struct.repetitioncounter = 1;
  
  // Initialize timer with basic parameter struct
  timer_init(TIMER_BLDC, &timerBldc_parameter_struct);
  timer_auto_reload_shadow_enable(TIMER_BLDC);
  
  // Set up the output channel parameter struct
  timer_channel_output_struct_para_init(&timerBldc_oc_parameter_struct);
  timerBldc_oc_parameter_struct.outputstate   = TIMER_CCX_DISABLE;
  timerBldc_oc_parameter_struct.outputnstate  = TIMER_CCXN_DISABLE;
  #if SIMPLEFOC_PWM_HIGHSIDE_ACTIVE_HIGH==false
    timerBldc_oc_parameter_struct.ocpolarity    = TIMER_OC_POLARITY_LOW;
    timerBldc_oc_parameter_struct.ocidlestate 	= TIMER_OC_IDLE_STATE_HIGH;
  #endif

  #if SIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH==false
    timerBldc_oc_parameter_struct.ocnpolarity 	= TIMER_OCN_POLARITY_LOW;
    timerBldc_oc_parameter_struct.ocnidlestate 	= TIMER_OCN_IDLE_STATE_HIGH;
  #endif

  // Configure the three phase channels and the sampling channel: PWM0, no fast mode, shadow register,
  // pulse length 0 for the phases (duty cycle zero)
  const uint16_t channels[4] = { channelA, channelB, channelC, TIMER_BLDC_CHANNEL_S };
  for (int i = 0; i < 4; i++) {
    timer_channel_output_config(TIMER_BLDC, channels[i], &timerBldc_oc_parameter_struct);
    timer_channel_output_mode_config(TIMER_BLDC, channels[i], TIMER_OC_MODE_PWM0);
    timer_channel_output_fast_config(TIMER_BLDC, channels[i], TIMER_OC_FAST_DISABLE);
    timer_channel_output_shadow_config(TIMER_BLDC, channels[i], TIMER_OC_SHADOW_ENABLE);
    timer_channel_output_pulse_value_config(TIMER_BLDC, channels[i], 0);
  }
  timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_S, SystemCoreClock/pwm_frequency * sampling_point);

  // Set up the break parameter struct
  timer_break_struct_para_init(&timerBldc_break_parameter_struct);
  timerBldc_break_parameter_struct.runoffstate      = TIMER_ROS_STATE_ENABLE;
  timerBldc_break_parameter_struct.ideloffstate     = TIMER_IOS_STATE_DISABLE;
  timerBldc_break_parameter_struct.protectmode	    = TIMER_CCHP_PROT_OFF;
  timerBldc_break_parameter_struct.deadtime 	      = _deadTimeRegister(dead_time_cycles);
  timerBldc_break_parameter_struct.outputautostate 	= TIMER_OUTAUTO_DISABLE;

  #ifdef TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN
    timerBldc_break_parameter_struct.breakstate	      = TIMER_BREAK_ENABLE;
    timerBldc_break_parameter_struct.breakpolarity	  = TIMER_BREAK_POLARITY_LOW;
  #endif

  // Configure the timer with the break parameter struct
  timer_break_config(TIMER_BLDC, &timerBldc_break_parameter_struct);

  // TIMER0 primary output function enable
  timer_primary_output_config(TIMER_BLDC, ENABLE);
   
  // Enable the timer and start PWM
  timer_enable(TIMER_BLDC);

  GD32DriverParams* params = new GD32DriverParams {
    .timers = { TIMER_BLDC, TIMER_BLDC, TIMER_BLDC, TIMER_BLDC, TIMER_BLDC, TIMER_BLDC },
    .channels = { channelA, channelA, channelB, channelB, channelC, channelC },
    .pwm_frequency  = pwm_frequency,
    .range          = SystemCoreClock/pwm_frequency,
    .dead_zone      = dead_zone,
    .interface_type = _HARDWARE_6PWM
  };
  
  return params; // success
}

// setting pwm to hardware pin - instead analogWrite()
void _setPwm(uint32_t timer_periph, uint16_t channel, uint32_t value, int resolution)
{  
  timer_channel_output_pulse_value_config(timer_periph,channel,value);
}


uint32_t _isChannelEnabled(uint32_t timer_periph, uint16_t channel)
{
  switch (channel)
  {
  /* configure TIMER_CH_0 */
  case TIMER_CH_0:
    return (TIMER_CHCTL2(timer_periph) & (uint32_t)TIMER_CHCTL2_CH0EN);
    break;
  /* configure TIMER_CH_1 */
  case TIMER_CH_1:
    return (TIMER_CHCTL2(timer_periph) & (uint32_t)TIMER_CHCTL2_CH1EN);
    break;
  /* configure TIMER_CH_2 */
  case TIMER_CH_2:
    return (TIMER_CHCTL2(timer_periph) & (uint32_t)TIMER_CHCTL2_CH2EN);
    break;
  /* configure TIMER_CH_3 */
  case TIMER_CH_3:
    return (TIMER_CHCTL2(timer_periph) & (uint32_t)TIMER_CHCTL2_CH3EN);
    break;
  default:
    return 0xFFFF;    // should never get here. Added to avoid compile warning
    break;
  }
}

void _setSinglePhaseState(PhaseState state, uint32_t timer_periph, uint16_t channel1, uint16_t channel2)
{
  _UNUSED(channel2);
  switch (state) {
    case PhaseState::PHASE_OFF:
      if (_isChannelEnabled(timer_periph, channel1) != 0) // disable timer channel only if enabled to avoid glitches
      {
        timer_channel_output_state_config(timer_periph,channel1,TIMER_CCX_DISABLE);
        timer_channel_complementary_output_state_config(timer_periph,channel1,TIMER_CCXN_DISABLE);
      }
      break;
    default:
      if (_isChannelEnabled(timer_periph, channel1) == 0) // enable timer channel only if disabled to avoid glitches
      {
        timer_channel_output_state_config(timer_periph,channel1,TIMER_CCX_ENABLE);
        timer_channel_complementary_output_state_config(timer_periph,channel1,TIMER_CCXN_ENABLE);
      }
      break;
  }
}

// - BLDC driver - 6PWM setting
// - hardware specific
void _writeDutyCycle6PWM(float dc_a, float dc_b, float dc_c, PhaseState* phase_state, void* params){
  switch(((GD32DriverParams*)params)->interface_type){
    case _HARDWARE_6PWM:
      // phase a
      _setSinglePhaseState(phase_state[0], ((GD32DriverParams*)params)->timers[0], ((GD32DriverParams*)params)->channels[0], ((GD32DriverParams*)params)->channels[1]);
      //if(phase_state[0] == PhaseState::PHASE_OFF) dc_a = 0.0f;
      _setPwm(((GD32DriverParams*)params)->timers[0], ((GD32DriverParams*)params)->channels[0], ((GD32DriverParams*)params)->range*dc_a, _PWM_RESOLUTION);
      // phase b
      _setSinglePhaseState(phase_state[1], ((GD32DriverParams*)params)->timers[2], ((GD32DriverParams*)params)->channels[2], ((GD32DriverParams*)params)->channels[3]);
      //if(phase_state[1] == PhaseState::PHASE_OFF) dc_b = 0.0f;
      _setPwm(((GD32DriverParams*)params)->timers[2], ((GD32DriverParams*)params)->channels[2], ((GD32DriverParams*)params)->range*dc_b, _PWM_RESOLUTION);
      // phase c
      _setSinglePhaseState(phase_state[2], ((GD32DriverParams*)params)->timers[4], ((GD32DriverParams*)params)->channels[4], ((GD32DriverParams*)params)->channels[5]);
      //if(phase_state[2] == PhaseState::PHASE_OFF) dc_c = 0.0f;
      _setPwm(((GD32DriverParams*)params)->timers[4], ((GD32DriverParams*)params)->channels[4], ((GD32DriverParams*)params)->range*dc_c, _PWM_RESOLUTION);
      
      // Update sampling point
      _setPwm(TIMER_BLDC, TIMER_BLDC_CHANNEL_S, ((GD32DriverParams*)params)->range*sampling_point, _PWM_RESOLUTION);
      break;
  }
  _UNUSED(phase_state);
}


#endif