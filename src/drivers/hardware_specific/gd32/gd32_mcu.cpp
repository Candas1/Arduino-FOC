#include "../../hardware_api.h"
#include "gd32_mcu.h"

#if defined(_GD32_DEF_)

// GD32 Arduino core only: must stay inside the guard, the other platforms don't have it
#include "gd32/PinNames.h"

  /*
  uint32_t timerAh,timerAl,timerBh,timerBl,timerCl,timerCh;
  uint8_t channelAh,channelAl,channelBh,channelBl,channelCh,channelCl;
  uint8_t chonah,chonal,chonbh,chonbl,chonch,choncl;
  */

  float sampling_point = SAMPLING_POINT;

void* _configure6PWM(long pwm_frequency, float dead_zone, const int pinA_h, const int pinA_l, const int pinB_h, const int pinB_l, const int pinC_h, const int pinC_l){
  if( !pwm_frequency || !_isset(pwm_frequency) ) pwm_frequency = _PWM_FREQUENCY; // default frequency 16khz
  else pwm_frequency = _constrain(pwm_frequency, 0, _PWM_FREQUENCY_MAX); // constrain to 50kHz max
  // center-aligned frequency is uses two periods
  pwm_frequency *=2;

  // timeout timer parameter structs
  timer_parameter_struct timeoutTimer_paramter_struct;

  // PWM timer Parameter structs
  timer_parameter_struct timerBldc_parameter_struct;	
  timer_break_parameter_struct timerBldc_break_parameter_struct;
  timer_oc_parameter_struct timerBldc_oc_parameter_struct;

  pin_function(DIGITAL_TO_PINNAME(pinA_h), GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, 2));
  pin_function(DIGITAL_TO_PINNAME(pinA_l), GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, 2));
  pin_function(DIGITAL_TO_PINNAME(pinB_h), GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, 2));
  pin_function(DIGITAL_TO_PINNAME(pinB_l), GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, 2));
  pin_function(DIGITAL_TO_PINNAME(pinC_h), GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, 2));
  pin_function(DIGITAL_TO_PINNAME(pinC_l), GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, 2));

  /*
  timerAh = pinmap_peripheral(DIGITAL_TO_PINNAME(pinA_h), PinMap_PWM);
  channelAh = GD_PIN_CHANNEL_GET(pinmap_function(DIGITAL_TO_PINNAME(pinA_h), PinMap_PWM));
  chonah = GD_PIN_CHON_GET(pinmap_function(DIGITAL_TO_PINNAME(pinA_h), PinMap_PWM));
  timerAl = pinmap_peripheral(DIGITAL_TO_PINNAME(pinA_l), PinMap_PWM);
  channelAl = GD_PIN_CHANNEL_GET(pinmap_function(DIGITAL_TO_PINNAME(pinA_l), PinMap_PWM));
  chonal = GD_PIN_CHON_GET(pinmap_function(DIGITAL_TO_PINNAME(pinA_l), PinMap_PWM));
  timerBh = pinmap_peripheral(DIGITAL_TO_PINNAME(pinB_h), PinMap_PWM);
  chonbh = GD_PIN_CHON_GET(pinmap_function(DIGITAL_TO_PINNAME(pinB_h), PinMap_PWM));
  channelBh = GD_PIN_CHANNEL_GET(pinmap_function(DIGITAL_TO_PINNAME(pinB_h), PinMap_PWM));
  timerBl = pinmap_peripheral(DIGITAL_TO_PINNAME(pinB_l), PinMap_PWM);
  chonbl = GD_PIN_CHON_GET(pinmap_function(DIGITAL_TO_PINNAME(pinB_l), PinMap_PWM));
  channelBl = GD_PIN_CHANNEL_GET(pinmap_function(DIGITAL_TO_PINNAME(pinB_l), PinMap_PWM));
  timerCh = pinmap_peripheral(DIGITAL_TO_PINNAME(pinC_h), PinMap_PWM);
  chonch = GD_PIN_CHON_GET(pinmap_function(DIGITAL_TO_PINNAME(pinC_h), PinMap_PWM));
  channelCh = GD_PIN_CHANNEL_GET(pinmap_function(DIGITAL_TO_PINNAME(pinC_h), PinMap_PWM));
  timerCl = pinmap_peripheral(DIGITAL_TO_PINNAME(pinC_l), PinMap_PWM);
  choncl = GD_PIN_CHON_GET(pinmap_function(DIGITAL_TO_PINNAME(pinC_l), PinMap_PWM));
  channelCl = GD_PIN_CHANNEL_GET(pinmap_function(DIGITAL_TO_PINNAME(pinC_l), PinMap_PWM));
  */

  //SIMPLEFOC_DEBUG("STM32-DRV: ERR: too many pins used");
  //return (GD32DriverParams*)SIMPLEFOC_DRIVER_INIT_FAILED;
  

  #ifdef TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN
    pin_function(DIGITAL_TO_PINNAME(TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN), GD_PIN_FUNCTION3(PIN_MODE_AF, PIN_OTYPE_PP, 2));
  #endif

  // dead time is set in nanoseconds
  uint32_t dead_time_ns = (float)(1e9f/pwm_frequency)*dead_zone;

  // #todo this needs to be converted
  // uint32_t dead_time = __LL_TIM_CALC_DEADTIME(SystemCoreClock, LL_TIM_GetClockDivision(HT->getHandle()->Instance), dead_time_ns);
  
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

  // Configure all three output channels with the output channel parameter struct
  timer_channel_output_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, &timerBldc_oc_parameter_struct);
  timer_channel_output_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, &timerBldc_oc_parameter_struct);
  timer_channel_output_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, &timerBldc_oc_parameter_struct);
  timer_channel_output_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_S, &timerBldc_oc_parameter_struct);
  
  // Set output channel PWM type to PWM0
  timer_channel_output_mode_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, TIMER_OC_MODE_PWM0);
  timer_channel_output_mode_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, TIMER_OC_MODE_PWM0);
  timer_channel_output_mode_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, TIMER_OC_MODE_PWM0);
  timer_channel_output_mode_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_S, TIMER_OC_MODE_PWM0);

  // Deactivate output channel fastmode
  timer_channel_output_fast_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, TIMER_OC_FAST_DISABLE);
  timer_channel_output_fast_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, TIMER_OC_FAST_DISABLE);
  timer_channel_output_fast_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, TIMER_OC_FAST_DISABLE);
  timer_channel_output_fast_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_S, TIMER_OC_FAST_DISABLE);

  // Deactivate output channel shadow function
  timer_channel_output_shadow_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, TIMER_OC_SHADOW_ENABLE);
  timer_channel_output_shadow_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, TIMER_OC_SHADOW_ENABLE);
  timer_channel_output_shadow_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, TIMER_OC_SHADOW_ENABLE);
  timer_channel_output_shadow_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_S, TIMER_OC_SHADOW_ENABLE);

  // Initialize pulse length with value 0 (pulse duty factor = zero)
  timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, 0);
  timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, 0);
  timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, 0);
  timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_S, SystemCoreClock/pwm_frequency * sampling_point);

  // Set up the break parameter struct
  timer_break_struct_para_init(&timerBldc_break_parameter_struct);
  timerBldc_break_parameter_struct.runoffstate      = TIMER_ROS_STATE_ENABLE;
  timerBldc_break_parameter_struct.ideloffstate     = TIMER_IOS_STATE_DISABLE;
  timerBldc_break_parameter_struct.protectmode	    = TIMER_CCHP_PROT_OFF;
  timerBldc_break_parameter_struct.deadtime 	      = DEAD_TIME; // 0~255
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
    .channels = { TIMER_BLDC_CHANNEL_G, TIMER_BLDC_CHANNEL_G, TIMER_BLDC_CHANNEL_B, TIMER_BLDC_CHANNEL_B, TIMER_BLDC_CHANNEL_Y, TIMER_BLDC_CHANNEL_Y },
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
      if (_isChannelEnabled(timer_periph, channel1) != 1) // disable timer channel only if enabled to avoid glitches
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