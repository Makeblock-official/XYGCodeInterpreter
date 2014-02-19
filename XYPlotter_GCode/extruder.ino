// Yep, this is actually -*- c++ -*-

#include "ThermistorTable.h"

#define EXTRUDER_FORWARD true
#define EXTRUDER_REVERSE false

//these our the default values for the extruder.
int extruder_speed = 128;
int extruder_target_celsius = 0;
int extruder_max_celsius = 0;
byte extruder_heater_low = 64;
byte extruder_heater_high = 255;
byte extruder_heater_current = 0;

//this is for doing encoder based extruder control
int extruder_rpm = 0;
long extruder_delay = 0;
int extruder_error = 0;
int last_extruder_error = 0;
int extruder_error_delta = 0;
bool extruder_direction = EXTRUDER_FORWARD;

volatile uint8_t *extruder_motor_dir_reg;
volatile uint8_t extruder_motor_dir_bitmask;

void init_extruder()
{
  //default to room temp.
  extruder_set_temperature(21);
	
  //setup our pins
  pinMode(EXTRUDER_MOTOR_DIR_PIN, OUTPUT);
  pinMode(EXTRUDER_MOTOR_SPEED_PIN, OUTPUT);
  pinMode(EXTRUDER_HEATER_PIN, OUTPUT);
  pinMode(EXTRUDER_FAN_PIN, OUTPUT);
	
  //initialize values
  getRegAndBitmask(&extruder_motor_dir_reg, &extruder_motor_dir_bitmask, EXTRUDER_MOTOR_DIR_PIN, OUTPUT);
  fastDigitalWrite(extruder_motor_dir_reg, extruder_motor_dir_bitmask, EXTRUDER_FORWARD);
  analogWrite(EXTRUDER_FAN_PIN, 0);
  analogWrite(EXTRUDER_HEATER_PIN, 0);
  analogWrite(EXTRUDER_MOTOR_SPEED_PIN, 0);	
}

void extruder_set_direction(bool direction)
{
  extruder_direction = direction;
  fastDigitalWrite(extruder_motor_dir_reg, extruder_motor_dir_bitmask, direction);
}

void extruder_set_speed(byte speed)
{
  analogWrite(EXTRUDER_MOTOR_SPEED_PIN, speed);  
}

void extruder_set_cooler(byte speed)
{
  analogWrite(EXTRUDER_FAN_PIN, speed);
}

void extruder_set_temperature(int temp)
{
  extruder_target_celsius = temp;
  extruder_max_celsius = (int)((float)temp * 1.1);
}

/**
*  Samples the temperature and converts it to degrees celsius.
*  Returns degrees celsius.
*/
int extruder_get_temperature()
{
#if EXTRUDER_THERMISTOR_PIN != -1
  return extruder_read_thermistor();
#endif
#if EXTRUDER_THERMOCOUPLE_PIN != -1
  return extruder_read_thermocouple();
#endif
}

#if EXTRUDER_THERMISTOR_PIN != -1
/*
* This function gives us the temperature from the thermistor in Celsius
*/
int extruder_read_thermistor()
{
	int raw = extruder_sample_temperature(EXTRUDER_THERMISTOR_PIN);

	int celsius = 0;
	byte i;

	for (i=1; i<NUMTEMPS; i++)
	{
		if (temptable[i][0] > raw)
		{
			celsius  = temptable[i-1][1] + 
				(raw - temptable[i-1][0]) * 
				(temptable[i][1] - temptable[i-1][1]) /
				(temptable[i][0] - temptable[i-1][0]);

			break;
		}
	}

        // Overflow: Set to last value in the table
        if (i == NUMTEMPS) celsius = temptable[i-1][1];
        // Clamp to byte
        if (celsius > 255) celsius = 255; 
        else if (celsius < 0) celsius = 0; 

	return celsius;
}
#endif

#if EXTRUDER_THERMOCOUPLE_PIN != -1
/*
* This function gives us the temperature from the thermocouple in Celsius
*/
int extruder_read_thermocouple()
{
  return ( 5.0 * extruder_sample_temperature(EXTRUDER_THERMOCOUPLE_PIN) * 100.0) / 1024.0;
}
#endif

/*
* This function gives us an averaged sample of the analog temperature pin.
*/
int extruder_sample_temperature(byte pin)
{
  int raw = 0;
	
  //read in a certain number of samples
  for (byte i=0; i<TEMPERATURE_SAMPLES; i++)
    raw += analogRead(pin);
		
  //average the samples
  raw = raw/TEMPERATURE_SAMPLES;

  //send it back.
  return raw;
}

/*!
  Manages motor and heater based on measured temperature:
  o If temp is too low, don't start the motor
  o Adjust the heater power to keep the temperature at the target
 */
void extruder_manage_temperature()
{
  //make sure we know what our temp is.
  int current_celsius = extruder_get_temperature();
  byte newheat = 0;
  
  //put the heater into high mode if we're not at our target.
  if (current_celsius < extruder_target_celsius)
    newheat = extruder_heater_high;
  
  //put the heater on low if we're at our target.
  else if (current_celsius < extruder_max_celsius)
    newheat = extruder_heater_low;
        
  // Only update heat if it changed
  if (extruder_heater_current != newheat) {
    extruder_heater_current = newheat;
    analogWrite(EXTRUDER_HEATER_PIN, extruder_heater_current);
  }
}
