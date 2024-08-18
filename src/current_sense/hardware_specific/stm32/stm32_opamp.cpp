#include "stm32_opamp.h"
#include "../../hardware_api.h"

#if defined(_STM32_DEF_)

#ifdef HAL_OPAMP_MODULE_ENABLED
OPAMP_HandleTypeDef hopamp1;
OPAMP_HandleTypeDef hopamp2;
OPAMP_HandleTypeDef hopamp3;

int _init_OPAMP(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *OPAMPx_Def){
  // could this be replaced with LL_OPAMP calls??
  hopamp->Instance = OPAMPx_Def;
  hopamp->Init.PowerMode = OPAMP_POWERMODE_HIGHSPEED;
  hopamp->Init.Mode = OPAMP_PGA_MODE;
  hopamp->Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_IO0;
  hopamp->Init.InternalOutput = DISABLE;
  hopamp->Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
  hopamp->Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0_BIAS;
  hopamp->Init.PgaGain = OPAMP_PGA_GAIN_16_OR_MINUS_15;
  hopamp->Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
  if (HAL_OPAMP_Init(hopamp) != HAL_OK)
  {
    SIMPLEFOC_DEBUG("STM32-CS: ERR: OPAMP init failed!");
    return -1;
  }

  if (HAL_OPAMP_SelfCalibrate(hopamp) != HAL_OK)
  {
    SIMPLEFOC_DEBUG("STM32-CS: ERR: OPAMP calibrate failed!");
    return -1;
  }

  if (HAL_OPAMP_Start(hopamp) != HAL_OK)
  {
    SIMPLEFOC_DEBUG("STM32-CS: ERR: OPAMP start failed!");
    return -1;
  }

  return 0;
}

int _init_OPAMPs(void){
  // Initialize Opamps
  if (_init_OPAMP(&hopamp1,OPAMP1) == -1) return -1;
	if (_init_OPAMP(&hopamp2,OPAMP2) == -1) return -1;
	if (_init_OPAMP(&hopamp3,OPAMP3) == -1) return -1;
  return 0;
}

#endif
#endif