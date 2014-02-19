#include <pins_arduino.h>

struct Move
{
  // Common to linear and arc moves
  unsigned char move_type;
  volatile bool direction[3];
  volatile float target_units[3];
  volatile float feedrate;
  volatile unsigned int step_delay;
  volatile long max_delta;
  volatile float accel_const;
  volatile long steps_to_accelerate;
  volatile float start_velocity[3];
  volatile float end_velocity[3];
  volatile long target_steps[3];
  bool continueMove;
  
  // Linear moves only
  volatile long delta_steps[3];
  
  // Arc moves only
  long initial_f;
  long arc_startPoint[2];
  char startOctant;
  char arc_direction;
};

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
#define POS 1
#define NEG -1
#define MOVE_LINEAR 0
#define MOVE_ARC 1
#define ARC_CLOCKWISE 1
#define ARC_COUNTERCLOCKWISE -1

// By XOR-ing 1 or -1 with the second of these values, its sign will be inverted - much faster than multiplication
#define ARC_XOR_CLOCKWISE 0x00
#define ARC_XOR_COUNTERCLOCKWISE 0xfe

// Offsets in table of units-based constants
#define MAX_ACCEL 3
#define MAX_DELTA_V 4
#define FAST_XY_FEEDRATE 5
#define FAST_Z_FEEDRATE 6
