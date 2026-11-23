# GD32F130 hoverboard split board

Torque control (`foc_current`) of a hoverboard in-wheel motor with its hall sensors and the low side current sense
of the split board TT-SD2.2 (GD32F130C8). Pins from
[Split_Hoverboard_SimpleFOC](https://github.com/Candas1/Split_Hoverboard_SimpleFOC) (`include/defines_2-0.h`).

## GD32 support in SimpleFOC

Only the **GD32F1x0** family (GD32F130/F150) has hardware specific code, with the
[ArduinoCore-GD32](https://github.com/CommunityGD32Cores/ArduinoCore-GD32) core:

| Feature | Status |
|---|---|
| 6PWM | hardware, complementary outputs of TIMER0 (the only timer with them on the F1x0), center aligned, dead time from `driver.dead_zone` |
| 3PWM, 2PWM, 1PWM, 4PWM | not specific: generic SimpleFOC functions (`analogWrite`) |
| Low side current sense | 2 or 3 phases, ADC inserted (injected) group triggered by TIMER0 channel 3, close to the middle of the low side ON time (`SAMPLING_POINT` 0.98) |
| Other GD32 families (F10x, F30x, E23x...) | not supported: 6PWM and the low side current sense fail at `init()` |

Options (build flags):
- `SIMPLEFOC_PWM_HIGHSIDE_ACTIVE_HIGH`, `SIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH`: polarity of the gate driver inputs.
  **The split board needs `-DSIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH=false`.**
- `TIMER_BLDC_EMERGENCY_SHUTDOWN_PIN=PB12`: break input of TIMER0 (split boards), off by default.
- `SIMPLEFOC_GD32_ADC_INTERRUPT`: read the currents in the ADC interrupt instead of the main loop.

## Build

PlatformIO:

```ini
[env:split_board]
platform = https://github.com/CommunityGD32Cores/platform-gd32.git
platform_packages = framework-arduinogd32@https://github.com/CommunityGD32Cores/ArduinoCore-GD32.git
board = genericGD32F130C8
framework = arduino
build_flags = -D SIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH=false
lib_deps = askuric/Simple FOC
```

Arduino IDE: add `https://raw.githubusercontent.com/CommunityGD32Cores/GD32Core-New/main/package_gd32_index.json` to the
boards manager, install "GigaDevice GD32", select "GD32F1x0 Generic series" and "GD32F130C8 (Generic)". The build flag
goes in a `platform.local.txt` next to the `platform.txt` of the core:

```
compiler.cpp.extra_flags=-DSIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH=false
```

Upload with an ST-Link on the SWD connector of the board (SWDIO, SWCLK, GND).

## Notes

- The current is measured on the R_ds_on of the low side MOSFETs: less accurate than a shunt and temperature dependent.
  It is only valid while the low side MOSFET conducts, so `current_sense.init()` runs after `motor.init()` (driver
  enabled during the offset calibration).
- `Serial` (USART0) is on PA9/PA10, which drive the motor: the example uses `Serial2` (USART1, PA2 TX, PA3 RX).
- No FPU: the floating point is in software, the loop is slow (`loopFOC` ~160 us at 72 MHz without monitoring).
- The board switches itself off when the power button is released unless PB2 is held high (done in `setup()`).
- The motor phase resistance is ~0.17 Ohm: 1 V is already ~6 A. Keep `voltage_limit` and `current_limit` low.
