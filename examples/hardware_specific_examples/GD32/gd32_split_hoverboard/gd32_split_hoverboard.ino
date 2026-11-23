/**
 * GD32F130 hoverboard split board: torque control (foc_current) with hall sensors and low side current sense
 *
 * HARDWARE CONFIGURATION:
 *  Board: hoverboard split board TT-SD2.2 (GD32F130C8), "master" side
 *  Motor: hoverboard in-wheel motor (15 pole pairs, ~0.17 Ohm phase resistance) with hall sensors
 *  Core: https://github.com/CommunityGD32Cores/ArduinoCore-GD32 (board GD32F130C8 Generic)
 *  Pins: Candas1/Split_Hoverboard_SimpleFOC, include/defines_2-0.h
 *
 * IMPORTANT:
 *  - The low side MOSFET drivers of this board are active low: build with -DSIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH=false
 *    (PlatformIO: build_flags; Arduino IDE: platform.local.txt, see README.md). With the default polarity both
 *    MOSFETs of a phase conduct at the same time.
 *  - Serial (USART0) uses PA9/PA10, which are PWM pins on this board: the example uses Serial2 (USART1, PA2 TX, PA3 RX).
 *  - The motor phase resistance is very low: 1 V is already ~6 A. Keep the voltage and current limits low.
 */
#include <SimpleFOC.h>

#if !defined(SIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH) || SIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH
#error "This board needs -DSIMPLEFOC_PWM_LOWSIDE_ACTIVE_HIGH=false (active low low side MOSFET drivers), see README.md"
#endif

// power latch: must be held high or the board switches itself off when the power button is released
#define SELF_HOLD_PIN PB2

// BLDC motor instance: 15 pole pairs, phase resistance [Ohm], KV rating [rpm/V], phase inductance [H]
BLDCMotor motor = BLDCMotor(15, 0.1664f, 17.0f, 0.00036858f);
// 6PWM on TIMER0:                 phase A high, low   phase B high, low   phase C high, low
BLDCDriver6PWM driver = BLDCDriver6PWM(PA10, PB15,      PA9, PB14,          PA8, PB13);
// low side current sense on the R_ds_on of the low side MOSFETs (8 mOhm, op-amp gain -2.5), 2 phases
LowsideCurrentSense current_sense = LowsideCurrentSense(0.008f, -2.5f, PB1, PB0, _NC);

// hall sensors
HallSensor sensor = HallSensor(PB11, PF1, PC14, 15);
void doA() { sensor.handleA(); }
void doB() { sensor.handleB(); }
void doC() { sensor.handleC(); }

// commander: T<amps> sets the target current, M for the motor commands
Commander command = Commander(Serial2);
void doTarget(char* cmd) { command.scalar(&motor.target, cmd); }
void doMotor(char* cmd) { command.motor(&motor, cmd); }

void setup() {
  pinMode(SELF_HOLD_PIN, OUTPUT);
  digitalWrite(SELF_HOLD_PIN, HIGH);

  Serial2.begin(115200);
  SimpleFOCDebug::enable(&Serial2);

  sensor.init();
  sensor.enableInterrupts(doA, doB, doC);
  motor.linkSensor(&sensor);

  // power supply voltage [V]
  driver.voltage_power_supply = 26;
  // 16 kHz (default of the GD32 driver)
  driver.pwm_frequency = 16000;
  // dead time, fraction of the PWM period: 0.01335 at 16 kHz = 0.83 us
  driver.dead_zone = 0.01335f;
  // below 100 % duty cycle: the low side MOSFETs must conduct long enough for the current sampling
  driver.voltage_limit = driver.voltage_power_supply * 0.95f;
  if (!driver.init()) {
    Serial2.println("Driver init failed!");
    return;
  }
  motor.linkDriver(&driver);
  current_sense.linkDriver(&driver);

  motor.torque_controller = TorqueControlType::foc_current;
  motor.controller = MotionControlType::torque;
  // voltage limit of the current controllers [V]
  motor.voltage_limit = 4;
  // current limit [A]
  motor.current_limit = 2;
  // the dead time eats ~0.7 V on this board: 1.5 V gives ~1 A for the sensor alignment
  motor.voltage_sensor_align = 1.5f;
  // current low pass filters [s]
  motor.LPF_current_q.Tf = 0.002f;
  motor.LPF_current_d.Tf = 0.002f;
  // no FPU (software floats, slow loop): current controllers tuned from R and L for a low bandwidth [Hz]
  motor.tuneCurrentController(100);

  motor.useMonitoring(Serial2);
  // also enables the driver
  motor.init();

  // R_ds_on current sense: the offsets are only valid while the low side MOSFETs conduct, so current_sense.init()
  // must run after motor.init() (driver enabled, 50 % duty cycle during the calibration)
  if (!current_sense.init()) {
    Serial2.println("Current sense init failed!");
    return;
  }
  // the hall sensors and the current sense phases are aligned with the motor phases
  current_sense.skip_align = true;
  motor.linkCurrentSense(&current_sense);

  // aligns the hall sensors (THE MOTOR MOVES)
  motor.initFOC();

  motor.target = 0;
  command.add('T', doTarget, "target current");
  command.add('M', doMotor, "motor");
  Serial2.println("Motor ready, set the target current with T<amps>, e.g. T0.5");
  _delay(1000);
}

void loop() {
  motor.loopFOC();
  motor.move();
  command.run();
}
