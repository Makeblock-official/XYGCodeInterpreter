// Code types - high nybble of byte in lookup table
#define CODE_TYPE_NONE 0
#define CODE_TYPE_INT 1<<4
#define CODE_TYPE_FLOAT 1<<5

// Code indices - low nybble of byte in lookup table
// Int code indices
#define G_CODE_INDEX 0
#define M_CODE_INDEX 1

// Float code indices
#define F_CODE_INDEX 0
#define I_CODE_INDEX 1
#define J_CODE_INDEX 2
#define P_CODE_INDEX 3
#define Q_CODE_INDEX 4
#define R_CODE_INDEX 5
#define S_CODE_INDEX 6
#define X_CODE_INDEX 7
#define Y_CODE_INDEX 8
#define Z_CODE_INDEX 9

// Code for unsupported code type
#define NULL_CODE_CODE 0

// Bitmasks in records of codes seen
#define G_CODE_SEEN 1<<G_CODE_INDEX
#define M_CODE_SEEN 1<<M_CODE_INDEX
#define F_CODE_SEEN 1<<F_CODE_INDEX
#define I_CODE_SEEN 1<<I_CODE_INDEX
#define J_CODE_SEEN 1<<J_CODE_INDEX
#define P_CODE_SEEN 1<<P_CODE_INDEX
#define Q_CODE_SEEN 1<<Q_CODE_INDEX
#define R_CODE_SEEN 1<<R_CODE_INDEX
#define S_CODE_SEEN 1<<S_CODE_INDEX
#define X_CODE_SEEN 1<<X_CODE_INDEX
#define Y_CODE_SEEN 1<<Y_CODE_INDEX
#define Z_CODE_SEEN 1<<Z_CODE_INDEX

// Lookup table used to determine what to do with each code - note stored in program memory.
// Although there a few bytes wasted here for non-implemented codes, this
// should be more than made up for by not having to implement a switch...case structure
// to process the codes - note we start at F, the first useful code
unsigned char codeTypes[21] PROGMEM = {
  CODE_TYPE_FLOAT | F_CODE_INDEX, // F
  CODE_TYPE_INT | G_CODE_INDEX, // G
  NULL_CODE_CODE, // H
  CODE_TYPE_FLOAT | I_CODE_INDEX, // I
  CODE_TYPE_FLOAT | J_CODE_INDEX, // J
  NULL_CODE_CODE, // K
  NULL_CODE_CODE, // L
  CODE_TYPE_INT | M_CODE_INDEX, // M
  NULL_CODE_CODE, // N
  NULL_CODE_CODE, // O
  CODE_TYPE_FLOAT | P_CODE_INDEX, // P
  CODE_TYPE_FLOAT | Q_CODE_INDEX, // Q
  CODE_TYPE_FLOAT | R_CODE_INDEX, // R
  CODE_TYPE_FLOAT | S_CODE_INDEX, // S
  NULL_CODE_CODE, // T
  NULL_CODE_CODE, // U
  NULL_CODE_CODE, // V
  NULL_CODE_CODE, // W
  CODE_TYPE_FLOAT | X_CODE_INDEX, // X
  CODE_TYPE_FLOAT | Y_CODE_INDEX, // Y
  CODE_TYPE_FLOAT | Z_CODE_INDEX, // Z
};

unsigned char intCodesSeen = 0; // 8 bits is enough for int codes
unsigned int floatCodesSeen = 0; // 16 bits is enough for float codes

int intVals[2];
float floatVals[10];

boolean absMode = false; // 0 = incremental; 1 = absolute

float units_based_constants_mm[7] = { X_STEPS_PER_MM, Y_STEPS_PER_MM, Z_STEPS_PER_MM, MAX_ACCEL_MM, MAX_DELTA_V_MM, FAST_XY_FEEDRATE_MM, FAST_Z_FEEDRATE_MM };
float units_based_constants_inch[7] = { X_STEPS_PER_INCH, Y_STEPS_PER_INCH, Z_STEPS_PER_INCH, MAX_ACCEL_INCH, MAX_DELTA_V_INCH, FAST_XY_FEEDRATE_INCH, FAST_Z_FEEDRATE_INCH };
float *units_based_constants = units_based_constants_mm; // Default to mm

float accel_const[3];

unsigned char dominantAxisTbl[8] = { X_AXIS, Y_AXIS, Y_AXIS, X_AXIS, X_AXIS, Y_AXIS, Y_AXIS, X_AXIS };
char dominantSignTbl[8] = { POS, NEG, NEG, NEG, NEG, POS, POS, POS };
char followingSignTbl[8] = { NEG, POS, NEG, NEG, POS, NEG, POS, POS };
char octantSignTbl[8] = { POS, NEG, POS, NEG, POS, NEG, POS, NEG};

//init our string processing
void init_process_string()
{
  //init our command
  for (byte i=0; i<COMMAND_SIZE; i++)
    command[i] = 0;
  serial_count = 0;
  bytes_received = false;

  idle_time = millis();
}

// Initial feedrate
float feedrate = units_based_constants[FAST_Z_FEEDRATE];

/* keep track of the last G code - this is the command mode to use
 * if there is no command in the current string 
 */
int lastGCode = -1;

//Read the string and execute instructions
void process_string(char instruction[], int size)
{
  //print("got  ");
  //println(instruction);
  
  intCodesSeen = 0;
  floatCodesSeen = 0;
  
  // Ignore lines starting with "/" and "(" as these are comments
  if (instruction[0] != '/' && instruction[0] != '(')
  {
    
    // Step through line looking for codes and values
    char *ind = instruction;
    char *lineEnd = instruction + size;
    char *valEnd;
    for (; ind < lineEnd; ind = valEnd ) {
      char *valStart = ind + 1;
      valEnd = valStart;
      char readChar = *ind;
      if (readChar >= 'F' && readChar <= 'Z') { // Code found
        char codeType = pgm_read_byte(&(codeTypes[readChar - 'F'])); // Code table starts at F - note reading from program space
        char codeIndex = codeType & 0xF; // Lower 4 bits contain code index
        if (codeType & CODE_TYPE_INT) {
          int res = (int)strtol(valStart, &valEnd, 10);
          if (valEnd - valStart) {
            intVals[codeIndex] = res;
            intCodesSeen |= 1<<codeIndex;
          }
          if(readChar=='Z'){
            
          println(res>0?"up":"down");
           analogWrite(5, res>0?150:0);
          }
        } else if (codeType & CODE_TYPE_FLOAT) {
          float res = (float)strtod(valStart, &valEnd);
          if (valEnd - valStart) {
            floatVals[codeIndex] = res;
            floatCodesSeen |= 1<<codeIndex;
          }
          if(readChar=='Z'){
          println(res>0?"up":"down");
           analogWrite(5, res>0?150:0);
          }
        }    
      }
    }
  }

  /* if no command was seen, but parameters were, then use the last G code as 
   * the current command
   */
  if ((!(intCodesSeen & (G_CODE_SEEN | M_CODE_SEEN))) && 
    ((intCodesSeen != 0) &&
    (lastGCode >= 0))
    )
  {
    /* yes - so use the previous command with the new parameters */
    intVals[G_CODE_INDEX] = lastGCode;
    intCodesSeen |= G_CODE_SEEN;
  }

  boolean moveReceived = false;
  
  // G-code received?
  if (intCodesSeen & G_CODE_SEEN)
  {
    lastGCode = intVals[G_CODE_INDEX]; 	/* remember this for future instructions */

    // Wait for there to be space in the move queue
    while (have_next_move) {}
    
    // Lock move queue while adding new move
    move_queue_lock_main = true;
    
    // Get X, Y, Z coordinates if supplied (and add to existing if in incremental mode)
    for (unsigned int i = 0; i < 3; i++) {
      next_move_ptr->target_units[i] = current_move_ptr->target_units[i];
      if (floatCodesSeen & (X_CODE_SEEN<<i)) {
        next_move_ptr->target_units[i] = floatVals[X_CODE_INDEX + i] + (absMode ? 0 : current_move_ptr->target_units[i]);
      }
    }
    
    // Get feedrate if supplied
    if (floatCodesSeen & F_CODE_SEEN )
      feedrate = floatVals[F_CODE_INDEX];
      
    next_move_ptr->feedrate = feedrate;

    //do something!
    switch (lastGCode)
    {
      //Rapid Positioning
      //Linear Interpolation
      //these are basically the same thing.
    case 0:
    case 1:
    {
      // No more information needed
      next_move_ptr->move_type = MOVE_LINEAR;
      moveReceived = true; // There is a move to be queued
    }
    break;
    // Clockwise arc
    case 2:
    // Counterclockwise arc
    case 3:
    {
      char arcDirection = (lastGCode == 3) ? ARC_COUNTERCLOCKWISE : ARC_CLOCKWISE;

      next_move_ptr->move_type = MOVE_ARC;
        
      long startPoint[2], endPoint[2];
      
      float rSquared = 0; // Can't be long because it would overflow for large radii
      
      // Use a loop to read I & J coords etc to save memory
      // startPoint and endPoint are both relative to the centre of the circle
      float centre_coord;
      for (unsigned int i = 0; i < 2; i++) {
        centre_coord = floatVals[I_CODE_INDEX + i]; // J follows I so can do it this way - note no check that they are actually supplied
        rSquared += square(centre_coord * units_based_constants[i]);
  
        startPoint[i] = -centre_coord * units_based_constants[i];
        endPoint[i] = (next_move_ptr->target_units[i] - (centre_coord + current_move_ptr->target_units[i])) * units_based_constants[i];
      }
       
      unsigned long r = (long)sqrt(rSquared); // This can be long
     
      // For counterclockwise, octants run counterclockwise too - to calculate this, invert X coord
      unsigned char startOctant = octant( arcDirection == ARC_CLOCKWISE ? startPoint[X_AXIS] : -startPoint[X_AXIS], startPoint[Y_AXIS]);
      unsigned char endOctant = octant( arcDirection == ARC_CLOCKWISE ? endPoint[X_AXIS] : -endPoint[X_AXIS], endPoint[Y_AXIS]);
      
      #define INVSQRT2 0.707106781
      float stepsPerOctant = INVSQRT2 * r; // Leave this as a float for now to minimise rounding errors
      unsigned char startOctantDominant = dominantAxisTbl[startOctant];
      unsigned char endOctantDominant = dominantAxisTbl[endOctant];
      
      // Get steps for start and end octants - this will be counting either from or to the end of the octant depending on its sign (and the direction)
      // Must reverse if octant sign is positive and clockwise, or vice versa (which is the same as octantSign != direction) - note reverse is true for endPoint
      long startOctantSteps = abs(startPoint[startOctantDominant]);
      if (octantSignTbl[startOctant] == POS)
        startOctantSteps = stepsPerOctant - startOctantSteps;
        
      long endOctantSteps = abs(endPoint[endOctantDominant]);
      if (octantSignTbl[endOctant] == NEG)
        endOctantSteps = stepsPerOctant - endOctantSteps;
      
      // Number of whole octants
      char wholeOctants = endOctant - startOctant;
     
      //if (arcDirection == ARC_COUNTERCLOCKWISE) wholeOctants = -wholeOctants;
      
      // First calculate offset between end and start points - this is only relevant if they fall in the same octant
      long stepOffset = endPoint[endOctantDominant] - startPoint[startOctantDominant];
      
      // Must reverse if dominant sign is negative and clockwise, or positive and counterclockwise (which is the same as dominantSign != direction)
      if (dominantSignTbl[startOctant] != arcDirection) stepOffset = -stepOffset;
      
      // Add 8 to wholeOctants in the following circumstances;
      // 1. The end octant is less than the start octant (vice versa for CCW) - for example if we start in octant 7 and end in octant 0 CW
      // 2. The start and end octants are the same and end position is less than or equal to the start position in the dominant axis
      //    of this octant (again vv for CCW) - this takes care of an arc which starts in an octant and continues right round to end in the same octant
      //    as well as the case of a complete circle ie startPos == endPos
      if ( wholeOctants < 0 || (endOctant == startOctant && stepOffset <= 0)) wholeOctants += 8;

      // Take away one - this seems cryptic but it is necessary
      wholeOctants--;
      // Total steps
      next_move_ptr->max_delta = startOctantSteps + (long)(stepsPerOctant * wholeOctants) + endOctantSteps;
      // Must count backwards in sign table for counterclockwise
      unsigned char signOctant = (arcDirection == ARC_CLOCKWISE) ? startOctant : 7 - startOctant;
      char arcDirectionXOR = (arcDirection == ARC_CLOCKWISE) ? ARC_XOR_CLOCKWISE : ARC_XOR_COUNTERCLOCKWISE; // Quick way of negating 1 or -1
      
      // Initial test point - one whole step on the dominant axis and one half on the following axis (hence "midpoint circle algorithm")
      float initialTestDominantCoord = startPoint[startOctantDominant] + (dominantSignTbl[signOctant] ^ arcDirectionXOR);
      float initialTestFollowingCoord = startPoint[1 - startOctantDominant] + 0.5 * (followingSignTbl[signOctant] ^ arcDirectionXOR);
      next_move_ptr->initial_f = (long)(square(initialTestDominantCoord) + square(initialTestFollowingCoord) - rSquared);
      
      float arcVelocityMultFactor = feedrate / r * arcDirection; // Factor by which to multiply coords to get velocities
      
      next_move_ptr->arc_direction = arcDirection;
      next_move_ptr->arc_startPoint[X_AXIS] = startPoint[X_AXIS];
      next_move_ptr->arc_startPoint[Y_AXIS] = startPoint[Y_AXIS];
      next_move_ptr->startOctant = startOctant;
      next_move_ptr->start_velocity[X_AXIS] = arcVelocityMultFactor * startPoint[Y_AXIS];
      next_move_ptr->start_velocity[Y_AXIS] = arcVelocityMultFactor * -startPoint[X_AXIS];
      next_move_ptr->end_velocity[X_AXIS] = arcVelocityMultFactor * endPoint[Y_AXIS];
      next_move_ptr->end_velocity[Y_AXIS] = arcVelocityMultFactor * -endPoint[X_AXIS];

      moveReceived = true; // There is a move to be queued
    }
    break;
    case 4: // Dwell
      delay((int)(floatVals[P_CODE_INDEX] * 1000));
      break;

    case 20: // Inches for Units
    case 21: // mm for Units
      while (moving) {} // Do not try and do this while moving
      units_based_constants = (lastGCode == 20) ? units_based_constants_inch : units_based_constants_mm;
      calculateAccelConstants();
      break;
      
    case 90: // Absolute Positioning
    case 91: // Incremental Positioning
      while (moving) {} // Do not try and do this while moving
      absMode = (lastGCode == 90) ? true : false;
      break;

    case 92: // Set absolute position - note that this must be G92 X?? Y?? Z?? and not just G92 alone which will in fact do nothing
      while (moving) {} // Do not try and do this while moving
      for (unsigned int i = 0; i < 3; i++) {
        if (floatCodesSeen & (X_CODE_SEEN<<i)) { // Can do this because Z follows Y follows X
          current_move_ptr->target_units[i] = floatVals[X_CODE_INDEX + i];
          current_move_ptr->target_steps[i] = current_steps[i] = units_based_constants[i] * floatVals[X_CODE_INDEX + i];
        }
      }
      break;

    default:
     print("huh? G");
      //println("Huh?");
      println(lastGCode);
    }
  }
  
  // Actually queue up the move (common to both linear and arc moves so it's here to make the code smaller)
  if (moveReceived) {

    calculate_deltas(lastGCode == 0); // Go at top speed for G0
      
    have_next_move = true;

    // If completely stopped, start moving again
    if (!moving) advance_move_queue();
  }
  
  // Clear move queue lock
  move_queue_lock_main = false;

  //find us an m code.
  if (intCodesSeen & M_CODE_SEEN)
  {
    switch (intVals[M_CODE_INDEX])
    {
    //turn extruder on, forward
    case 101:
      extruder_set_direction(1);
      extruder_set_speed(extruder_speed);
      break;

    //turn extruder on, reverse
    case 102:
      extruder_set_direction(0);
      extruder_set_speed(extruder_speed);
      break;

    //turn extruder off
    case 103:
      extruder_set_speed(0);
      break;

    //custom code for temperature control
    case 104:
      if (floatCodesSeen & S_CODE_SEEN) {
        extruder_set_temperature((int)floatVals[S_CODE_INDEX]);

        //warmup if we're too cold.
	while (extruder_get_temperature() < extruder_target_celsius)
	{
	  extruder_manage_temperature();
          show_temperature();
	  delay(1000);
	}
      }
      break;

    //custom code for temperature reading
    case 105:
      show_temperature();
      break;

    //turn fan on
    case 106:
      extruder_set_cooler(255);
      break;

    //turn fan off
    case 107:
      extruder_set_cooler(0);
      break;

    //set max extruder speed, 0-255 PWM
    case 108:
    if (floatCodesSeen & S_CODE_SEEN)
      extruder_speed = (int)floatVals[S_CODE_INDEX];
      break;

    default:
      //println("Huh?");
      print("Huh? M");
      println(intVals[M_CODE_INDEX]);
    }
  }

  //tell our host we're done
  println("ok");
}

// Lookup table for calculating octant of circle - the two nybbles of each byte each contain a separate value
#define NV 0xf // Not Valid - some values will never be read, since for example x > 0 and x==0 can clearly not both be true
unsigned char octantTbl[19] PROGMEM =  {4 |  5<<4,
                                        5 | NV<<4,
                                        7 |  7<<4,
                                        6 | NV<<4,
                                        3 |  3<<4,
                                        2 | NV<<4,
                                        0 |  1<<4,
                                        1 | NV<<4,
                                       NV | NV<<4,
                                        6 | NV<<4,
                                       NV | NV<<4,
                                       NV | NV<<4,
                                       NV | NV<<4,
                                        2 | NV<<4,
                                       NV | NV<<4,
                                       NV | NV<<4,
                                        4 | NV<<4,
                                       NV | NV<<4,
                                        0 | NV<<4};

// Given coordinates of a point relative to the centre of
// a circle, work out its octant (clockwise starting on positive y axis)
// the tricky thing is that we must be constistent about edge conditions
// what is done here is that on the edge is taken as in the following (looking clockwise) octant
unsigned char octant(long x, long y) {
  
  // Basically we use a variety of tests to set bits, which collectively form an index to the byte array above - this is both fast and relatively memory efficient
  unsigned char octantTblIndex = 0;
  if (abs(x) > abs(y)) octantTblIndex |= _BV(0);
  if (y > 0) octantTblIndex |= _BV(1);
  if (x > 0) octantTblIndex |= _BV(2);
  if (y == 0) octantTblIndex |= _BV(3);
  if (x == 0) octantTblIndex |= _BV(4);
  
  unsigned char octant = pgm_read_byte(&(octantTbl[octantTblIndex]));
  
  // The final test (x==y) is used to select which nybble of the returned byte to use -
  // we are REALLY pushed for space here and this halves the size of the lookup table
  if (x==y) octant >>= 4;
  return octant & 0xf;
}

void show_temperature() {
  print("T:");
  println(extruder_get_temperature());
}
