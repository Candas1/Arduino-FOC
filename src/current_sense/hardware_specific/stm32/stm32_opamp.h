#ifndef STM32_OPAMP_DEF
#define STM32_OPAMP_DEF
#include "Arduino.h"
#include "../../../communication/SimpleFOCDebug.h"


#ifdef HAL_OPAMP_MODULE_ENABLED
int _init_OPAMP(OPAMP_HandleTypeDef *hopamp, OPAMP_TypeDef *OPAMPx_Def);
int _init_OPAMPs(void);
#endif

#endif