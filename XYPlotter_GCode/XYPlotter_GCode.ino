#include "reprap_new_firmware.h"


#define COMMAND_SIZE 128
char command[COMMAND_SIZE];
byte serial_count;
int no_data = 0;
long idle_time;
boolean comment = false;
boolean bytes_received = false;

long current_steps[3];

Move move_queue[2];
volatile Move *current_move_ptr, *next_move_ptr;

// Following all need to be volatile because they are
// being both read and set by the main and interrupt handler routines
volatile bool have_next_move = false;
volatile bool move_queue_lock_main = false;
volatile bool moving = false;
int motorDrc = 4; //M1
int motorPwm = 5; //M1
unsigned int timer1LoadValue, nextTimer1LoadValue;
void setup()
{
  init_steppers();
  init_extruder();
  init_process_string();
  calculateAccelConstants();
  SetupTimer1();
  
  pinMode(motorDrc, OUTPUT);
  digitalWrite(motorDrc, HIGH);
  pinMode(motorPwm, OUTPUT);
  analogWrite(motorPwm, 0);
  
  Serial.begin(9600);
  println("start");
}

void loop()
{
  char c;
  
  //keep it hot!
  //extruder_manage_temperature();

  //read in characters if we got them.
  if (Serial.available() > 0)
  {
    c = Serial.read();
    no_data = 0;

  
    //newlines are ends of commands.
    if (c != '\n')
    {
      // Start of comment - ignore any bytes received from now on
      if (c == '(')
        comment = true;

      // If we're not in comment mode, add it to our array.
      if (!comment)
      {
        command[serial_count] = c;
        serial_count++;
      }
      if (c == ')')
        comment = false; // End of comment - start listening again
    }
    bytes_received = true;
    idle_time = millis();
  }
  //mark no data if nothing heard for 100 milliseconds
  else
  {
    if ((millis() - idle_time) >= 100)
    {
      no_data++;
      idle_time = millis();
    }
  }

  //if theres a pause or we got a real command, do it
  if (bytes_received && (c == '\n' || no_data ))
  {
    //process our command!
    //println(command);
    process_string(command, serial_count);

    //clear command.
    init_process_string();
  }
}
