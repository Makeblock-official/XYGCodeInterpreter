// define the parameters of our machine.
// belt drive
//#define X_STEPS_PER_INCH 200.0
//#define X_STEPS_PER_MM   7.874

// all thread leadscrew
#define X_STEPS_PER_INCH 8000.0
#define X_STEPS_PER_MM   3

// ACME leadscrew
//#define X_STEPS_PER_INCH 6400.0
//#define X_STEPS_PER_MM   251.968

#define Y_STEPS_PER_INCH 8000.0
#define Y_STEPS_PER_MM   3

#define Z_STEPS_PER_INCH 8000.0
#define Z_STEPS_PER_MM   3

//our maximum feedrates in units/minute
#define FAST_XY_FEEDRATE_INCH 20 //300
#define FAST_Z_FEEDRATE_INCH  15
#define FAST_XY_FEEDRATE_MM 360
#define FAST_Z_FEEDRATE_MM  360

// Maximum acceleration in units/minute/second
// E.g. for 300.0 machine would accelerate to 150units/minute in 0.5sec etc.
#define MAX_ACCEL_INCH 50.0 //500.0
#define MAX_ACCEL_MM 1200.0

// Maximum change in velocity per axis - if the change in velocity at the start
// of the next move is greater than this for at least one axis, we will decelerate
// to a stop before commencing the move, otherwise we will keep going
// value is units/minute
#define MAX_DELTA_V_INCH 5.0
#define MAX_DELTA_V_MM 100.0

// Set to one if endstop outputs are inverting (ie: 1 means open, 0 means closed)
// RepRap opto endstops are *not* inverting.
#define ENDSTOPS_INVERTING 1

// Optionally disable max endstops to save pins or wiring
#define ENDSTOPS_MIN_ENABLED 0
#define ENDSTOPS_MAX_ENABLED 0

// How many temperature samples to take.  each sample takes about 100 usecs.
#define TEMPERATURE_SAMPLES 5

// The *_ENABLE_PIN signals are active high as default. Define this
// to one if they should be active low instead (e.g. if you're using different
// stepper boards).
// RepRap stepper boards are *not* inverting.
#define INVERT_ENABLE_PINS 0

// If you use this firmware on a cartesian platform where the
// stepper direction pins are inverted, set these defines to 1
// for the axes which should be inverted.
// RepRap stepper boards are *not* inverting.
#define INVERT_DIR 1 // CHANGED CM - ONLY ONE FOR ALL AXES

#define STEPPERS_ALWAYS_ON 0


/****************************************************************************************
* digital i/o pin assignment
*
* this uses the undocumented feature of Arduino - pins 14-19 correspond to analog 0-5
****************************************************************************************/

//cartesian bot pins
#define X_STEP_PIN 2
#define X_DIR_PIN 8
#define X_ENABLE_PIN 14
#define X_MIN_PIN 3
#define X_MAX_PIN 3

#define Y_STEP_PIN 13
#define Y_DIR_PIN 12  
#define Y_ENABLE_PIN 15
#define Y_MIN_PIN 3
#define Y_MAX_PIN 3

#define Z_STEP_PIN 19
#define Z_DIR_PIN 19
#define Z_ENABLE_PIN 19
#define Z_MIN_PIN 3
#define Z_MAX_PIN 3

//extruder pins
// NOTE - USING Timer1 FOR STEPPER TIMER SO CAN'T USER PINS 9 OR 10 FOR PWM
// OUTPUT (EXTRUDER_MOTOR_SPEED_PIN, EXTRUDER_HEATER_PIN, OR EXTRUDER_FAN_PIN)
#define EXTRUDER_MOTOR_SPEED_PIN   19
#define EXTRUDER_MOTOR_DIR_PIN     19
#define EXTRUDER_HEATER_PIN        19
#define EXTRUDER_FAN_PIN           19
#define EXTRUDER_THERMISTOR_PIN    -1  //a -1 disables thermistor readings
#define EXTRUDER_THERMOCOUPLE_PIN  -1 //a -1 disables thermocouple readings
