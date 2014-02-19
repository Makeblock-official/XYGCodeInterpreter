#define MOVE_PHASE_ACCEL 0
#define MOVE_PHASE_CONST_VELOCITY 1
#define MOVE_PHASE_DECEL 2

// All of the following must be
// volatile because they are being set and read in both the
// main routine and in the interrupt handler
volatile long step_counter[3];
volatile bool move_finished = true;
volatile long sequence_step_number;
volatile long sequence_total_steps;
volatile long sequence_steps_to_accelerate;
volatile long sequence_steps_accelerated;
volatile byte move_phase;
volatile long last_t;
volatile bool continue_move = false;
volatile bool inTimer1InterruptRoutine = false;

// Output/input registers and bitmasks
volatile uint8_t *step_output_reg[3];
volatile uint8_t step_bitmask[3];
volatile uint8_t *direction_output_reg[3];
volatile uint8_t direction_bitmask[3];
volatile uint8_t *enable_output_reg[3];
volatile uint8_t enable_bitmask[3];
volatile uint8_t *min_input_reg[3];
volatile uint8_t min_bitmask[3];
volatile uint8_t *max_input_reg[3];
volatile uint8_t max_bitmask[3];



#if INVERT_ENABLE_PINS == 1
#define ENABLE_ON LOW
#else
#define ENABLE_ON HIGH
#endif

volatile long arcAxis[2];
volatile long arcStep, arcF;
volatile unsigned char arcOldDominantAxis;
volatile char arcOldFollowingSign, arcDirectionXORValue, arcOctant;

void init_steppers()
{
#if STEPPERS_ALWAYS_ON == 0
  // Turn them off to start.
   disable_steppers();
#endif
/*
  // hack to open limit switches
  pinMode(X_MIN_PIN, OUTPUT);
  digitalWrite(X_MIN_PIN,LOW);
  pinMode(X_MAX_PIN, OUTPUT);
  digitalWrite(X_MAX_PIN,LOW);
  pinMode(Y_MIN_PIN, OUTPUT);
  digitalWrite(Y_MIN_PIN,LOW);
  pinMode(Y_MAX_PIN, OUTPUT);
  digitalWrite(Y_MAX_PIN,LOW);
  */
  unsigned char step_pins[3] = { X_STEP_PIN, Y_STEP_PIN, Z_STEP_PIN };
  unsigned char dir_pins[3] = { X_DIR_PIN, Y_DIR_PIN, Z_DIR_PIN };
  unsigned char enable_pins[3] = { X_ENABLE_PIN, Y_ENABLE_PIN, Z_ENABLE_PIN };
  unsigned char min_pins[3] = { X_MIN_PIN, Y_MIN_PIN, Z_MIN_PIN };
  unsigned char max_pins[3] = { X_MAX_PIN, Y_MAX_PIN, Z_MAX_PIN };

  // Set up initial pointers to move queue - these will
  // be continuously rotated in operation
  current_move_ptr = &move_queue[0];
  next_move_ptr = &move_queue[1];

  // Use loop to set up axes to save memory (16K baby!)
  for (unsigned char i = 0; i < 3; i++) {
    
    // Precompute ports and bitmasks for output pins
    // to save time during interrupt routine
    getRegAndBitmask(&step_output_reg[i], &step_bitmask[i], step_pins[i], OUTPUT);
    getRegAndBitmask(&direction_output_reg[i], &direction_bitmask[i], dir_pins[i], OUTPUT);
    getRegAndBitmask(&enable_output_reg[i], &enable_bitmask[i], enable_pins[i], OUTPUT);
    getRegAndBitmask(&min_input_reg[i], &min_bitmask[i], min_pins[i], INPUT);
    getRegAndBitmask(&max_input_reg[i], &max_bitmask[i], max_pins[i], INPUT);
    
    // Set mode for output pins
    pinMode(step_pins[i], OUTPUT);
    pinMode(dir_pins[i], OUTPUT);
    pinMode(enable_pins[i], OUTPUT);
    
    // Set mode for input pins (if applicable)
#if ENDSTOPS_MIN_ENABLED == 1
    pinMode(min_pins[i], INPUT);
#endif
#if ENDSTOPS_MAX_ENABLED == 1
    pinMode(max_pins[i], INPUT);
#endif

    // Start at zero
    current_move_ptr->target_units[i] = 0.0;
    current_move_ptr->target_steps[i] = 0;
    current_steps[i] = 0;
  }
  current_move_ptr->step_delay = 0;
}

// Get output/input register and bitmask for a given pin
void getRegAndBitmask(volatile uint8_t** reg, volatile uint8_t* bitmask, uint8_t pin, uint8_t mode) {
  uint8_t port = digitalPinToPort(pin);
  *reg = (mode == OUTPUT) ? portOutputRegister(port) : portInputRegister(port);
  *bitmask = digitalPinToBitMask(pin);  
}

void calculate_deltas(bool max_speed)
{
  float distance_squared = 0.0;
  unsigned char max_axis = 0;
  long max_axis_delta = 0;
  
  // Loop through axes calculating steps from units and finding deltas
  // and the largest axis (last two only applicable for linear moves)
  for (unsigned char i = 0; i < 3; i++) {
    float delta_units = abs(next_move_ptr->target_units[i] - current_move_ptr->target_units[i]);
    distance_squared += square(delta_units);
    next_move_ptr->target_steps[i] = next_move_ptr->target_units[i] * units_based_constants[i];
    
    long delta_steps = abs(current_move_ptr->target_steps[i] - next_move_ptr->target_steps[i]);
    next_move_ptr->delta_steps[i] = delta_steps;
    if (delta_steps > max_axis_delta) {
      max_axis_delta = delta_steps;
      max_axis = i;
    }
    next_move_ptr->direction[i] = (next_move_ptr->target_steps[i] >= current_move_ptr->target_steps[i]);
  }

  float feedrate, master_axis_feedrate, distance;

  feedrate = next_move_ptr->feedrate;
  
  if (next_move_ptr->move_type == MOVE_LINEAR) {
    
    // Max delta
    next_move_ptr->max_delta = max_axis_delta;
    
    // How long is the move?
    distance = sqrt( distance_squared );
    
    if (max_speed) {
      master_axis_feedrate = feedrate = units_based_constants[(next_move_ptr->delta_steps[Z_AXIS]) ? FAST_Z_FEEDRATE : FAST_XY_FEEDRATE];
      
      // Step delay will be calculate below (same calculation as for arc move)
    } else {
      master_axis_feedrate = ((float)max_axis_delta / units_based_constants[max_axis]) / distance * feedrate; // Feedrate in master (fastest) axis
      //master_axis_feedrate = 
      // Calculate delay between steps in microseconds.  this is sort of tricky, but not too bad.
      // the formula has been condensed to save space.  here it is in english:
      // (feedrate is in units/minute)
      // distance / feedrate * 60000000.0 = move duration in microseconds
      // move duration / master_steps = time between steps for master axis.
      next_move_ptr->step_delay = (unsigned int)(((distance * 60000000.0) / feedrate) / max_axis_delta);
    }
         
  } else { // MOVE_ARC
  
    // Max delta (total steps) already calculated
    max_axis = X_AXIS;
    master_axis_feedrate = feedrate;
  }
  
  // THIS WILL NOT WORK IF units ARE NOT THE SAME FOR ALL AXES!
  if (next_move_ptr->move_type == MOVE_ARC || max_speed) next_move_ptr->step_delay = 60000000.0 / (feedrate * units_based_constants[X_AXIS]);
  
  // Calculate signed velocities for each axis - this will
  // be used to see if the current move can be continued without stopping
  // Start and end velocities are identical for linear moves
  // Note for arc moves this is only done for the Z axis, the other two are calculated elsewhere
  for (unsigned char i = (next_move_ptr->move_type == MOVE_LINEAR ? 0 : 2); i < 3; i++) {
    float scale_factor = ((float)next_move_ptr->delta_steps[i] / next_move_ptr->max_delta);
    if (!next_move_ptr->direction[i]) scale_factor = -scale_factor;
    next_move_ptr->start_velocity[i] = next_move_ptr->end_velocity[i] = scale_factor * master_axis_feedrate;
  }
  
  // If any axis is out of tolerance for change in velocity from the current move, decelerate at end of current
  // move and accelerate into the next. If they are all within tolerance, continue in one smooth movement
  bool continueMove = true;
  for (unsigned char i = 0; i < 3; i++) {
    if (abs(current_move_ptr->end_velocity[i] - next_move_ptr->start_velocity[i]) >= units_based_constants[MAX_DELTA_V]) continueMove = false;
  }
  next_move_ptr->continueMove = continueMove;
  
  next_move_ptr->accel_const = accel_const[max_axis];
  float steps_per_unit = units_based_constants[max_axis];
  
  // Calculate the number of steps it will take to accelerate to full speed
  // This is done as follows (assuming starting from rest):
  // v = total_distance/distance_master_axis * feedrate since if we are moving at say, 100mm/min in two axes, each individual
  // axis will be moving slower than this 
  // v = a * t
  // integrating: x = a * t * t / 2
  // substitute in first equation gives: x = v^2 / ( 2 * a )
  // x = steps / steps_per_unit (i.e. x_units, y_units, or z_units depending on which is dominant axis)
  // subsituting gives: steps = v^2 * steps_per_unit /  (2 * a)
  // but v is in units/min and a is in units/min/sec, so must multiply a by 60 to get units/min/min gives: steps = v^2 * step_per_unit / (120 * a)
  next_move_ptr->steps_to_accelerate = (int)floor(square(master_axis_feedrate) * steps_per_unit / ( 120 * units_based_constants[MAX_ACCEL] ));
}

// Enable steppers for linear movement
void enable_steppers_linear()
{
  // Enable steppers only for axes which are moving
  // taking account of the fact that some or all axes
  // may share an enable line (check using macros at
  // compile time to avoid needless code)
  fastDigitalWrite( enable_output_reg[X_AXIS], enable_bitmask[X_AXIS],
    (current_move_ptr->delta_steps[X_AXIS] == 0
#if X_ENABLE_PIN == Y_ENABLE_PIN
    && current_move_ptr->delta_steps[Y_AXIS] == 0
#endif
#if X_ENABLE_PIN == Z_ENABLE_PIN
    && current_move_ptr->delta_steps[Z_AXIS] == 0
#endif
#if STEPPERS_ALWAYS_ON == 0
    ) ? !ENABLE_ON : ENABLE_ON);
#else
    ) ? ENABLE_ON : ENABLE_ON);
#endif
    
#if Y_ENABLE_PIN != X_ENABLE_PIN
  fastDigitalWrite( enable_output_reg[Y_AXIS], enable_bitmask[Y_AXIS],
    (current_move_ptr->delta_steps[Y_AXIS] == 0
#if Y_ENABLE_PIN == Z_ENABLE_PIN
    && current_move_ptr->delta_steps[Z_AXIS] == 0
#endif
#if STEPPERS_ALWAYS_ON == 0
    ) ? !ENABLE_ON : ENABLE_ON);
#else
    ) ? ENABLE_ON : ENABLE_ON);
#endif
#endif

#if Z_ENABLE_PIN != X_ENABLE_PIN && Z_ENABLE_PIN != Y_ENABLE_PIN
  fastDigitalWrite( enable_output_reg[Z_AXIS], enable_bitmask[Z_AXIS],
    (current_move_ptr->delta_steps[Z_AXIS] == 0 ) ? !ENABLE_ON : ENABLE_ON );
#endif
}

// Enable steppers for arc movement
void enable_steppers_arc() {
  
  // Always enable both X and Y steppers
  fastDigitalWrite( enable_output_reg[X_AXIS], enable_bitmask[X_AXIS], ENABLE_ON);
  fastDigitalWrite( enable_output_reg[Y_AXIS], enable_bitmask[Y_AXIS], ENABLE_ON);
  
  // Enable Z stepper if there is any movement in Z-axis (ie helical) - note this is pointless if it shares an enable line with either the X- or Y- axes
#if Z_ENABLE_PIN != X_ENABLE_PIN && Z_ENABLE_PIN != Y_ENABLE_PIN
  fastDigitalWrite( enable_output_reg[Z_AXIS], enable_bitmask[Z_AXIS], 
    (current_move_ptr->delta_steps[Z_AXIS] == 0) ? !ENABLE_ON : ENABLE_ON );
#endif
}

// Disable all steppers
void disable_steppers()
{
  for (unsigned int i = 0; i < 3; i++) {
    fastDigitalWrite( enable_output_reg[i], enable_bitmask[i], !ENABLE_ON);
  }
}

// Set up Timer1 to be used to control stepping
void SetupTimer1()
{
  // Clear Timer1 Output Compare Match interrupt enable
  TIMSK1 = 0<<OCIE1A;
  
  // Settings - mode 8 (Clear Timer on Compare match), divide by 8 (thus 2MHz clock since system clock is 16MHz)
  TCCR1A = 0;
  TCCR1B = 1<<WGM12 | 0<<CS12 | 1<<CS11 | 0<<CS10;
}

// Calculate Timer1 delay in microseconds
inline unsigned int Timer1Delay(unsigned int delay) {
  return delay<<1; // 2MHz clock, so multiply by two to get clock cycles for given delay
}

// Timer1 output compare match A interrupt vector handler
// send step pulses to motor controllers, do
// acceleration calculations, and advance move
// queue if done
ISR(TIMER1_COMPA_vect) {

  // Because we re-enable interrupts below, we must manually check
  // to prevent reentrancy if the routine takes too long
  if (inTimer1InterruptRoutine) return;
  inTimer1InterruptRoutine = true;
  
  // Enable interrupts - this is not normally a good
  // idea in an interrupt routine, but we want
  // to be interruptable so that any incoming serial
  // bytes do not get lost (if stepping gets delayed
  // a bit that is unfortunate, if we lose serial
  // data it's a disaster)
  sei();
  
  boolean can_step = false;

  // Axis loop - note for arc moves only Z axis moved in this way (to produce helical motion)
  // Of course it would be better to unroll this loop but there is no space for this
  for (unsigned char i = (current_move_ptr->move_type == MOVE_LINEAR ? 0 : 2); i < 3; i++) {
      
    // Check ok to step - that we are not at the end position, and that (if applicable) the endstops have not been hit
    if (current_steps[i] != current_move_ptr->target_steps[i] && canStep(i, current_move_ptr->direction[i])) {
      can_step = true; // At least one axis still moving
        
      step_counter[i] += current_move_ptr->delta_steps[i];
      if (step_counter[i] > 0 ) {
        *step_output_reg[i] |= step_bitmask[i]; // Set step line high
        step_counter[i] -= current_move_ptr->max_delta;
          
        if (current_move_ptr->direction[i])
          current_steps[i]++;
        else
          current_steps[i]--;
      }
    }
  }
  // Movement in X and Y axes for arc moves calculated here
  if (current_move_ptr->move_type == MOVE_ARC) {
    
    unsigned char arcDominantAxis = dominantAxisTbl[arcOctant];
    unsigned char arcFollowingAxis = 1 - arcDominantAxis;
    unsigned char signOctant = (arcDirectionXORValue == ARC_XOR_CLOCKWISE) ? arcOctant : 7 - arcOctant; // Need to reverse order of signs tables for counterclockwise
    char arcDominantSign = dominantSignTbl[signOctant] ^ arcDirectionXORValue; // XOR with 0xfe will invert sign from -1 to 1 or vice versa
    char arcFollowingSign = followingSignTbl[signOctant] ^ arcDirectionXORValue;
    char arcOctantSign = octantSignTbl[arcOctant];
    long arcDominantPos = arcAxis[arcDominantAxis];
    long arcFollowingPos = arcAxis[arcFollowingAxis];
    if (arcDominantSign == NEG) arcDominantPos = -arcDominantPos;
    if (arcFollowingSign == NEG) arcFollowingPos = -arcFollowingPos;
    
    // Correct test after dominant axis change (happens at 0, PI/2, PI, 3PI/2)
    if (arcDominantAxis != arcOldDominantAxis) {
      arcF += arcDominantPos - arcFollowingPos;
      
    // Correct test after change in sign of following axis (happens at PI/4, 3PI/4, 5PI/4, 7PI/4)
    } else if (arcFollowingSign != arcOldFollowingSign) {
      arcF += arcFollowingPos + arcFollowingPos;
    }
    
    arcOldDominantAxis = arcDominantAxis;
    arcOldFollowingSign = arcFollowingSign;
    if (((arcF >= 0) && arcOctantSign == POS) || ((arcF < 0) && arcOctantSign == NEG)) { // Make diagonal step, so advance following axis
      
      arcF += arcFollowingPos + arcFollowingPos + 2;
      arcAxis[arcFollowingAxis] += arcFollowingSign;
      
      // Set direction line for following axis
#if INVERT_DIR == 1
      fastDigitalWrite(direction_output_reg[arcFollowingAxis], direction_bitmask[arcFollowingAxis], arcFollowingSign == NEG);
#else
      fastDigitalWrite(direction_output_reg[arcFollowingAxis], direction_bitmask[arcFollowingAxis], arcFollowingSign == POS);
#endif

      // Check following axis not at limits and set step line high
      if (canStep(arcFollowingAxis, arcFollowingSign == POS)){
        *step_output_reg[arcFollowingAxis] |= step_bitmask[arcFollowingAxis];
      }
      
    }
    arcF += arcDominantPos + arcDominantPos + 3;
    arcAxis[arcDominantAxis] += arcDominantSign;
    
    // Set direction line for dominant axiss
#if INVERT_DIR == 1
    fastDigitalWrite(direction_output_reg[arcDominantAxis], direction_bitmask[arcDominantAxis], arcDominantSign == NEG);
#else
    fastDigitalWrite(direction_output_reg[arcDominantAxis], direction_bitmask[arcDominantAxis], arcDominantSign == POS);
#endif

    // Check dominant axis not at limits and set step line high
    if (canStep(arcDominantAxis, arcDominantSign == POS)){
      *step_output_reg[arcDominantAxis] |= step_bitmask[arcDominantAxis];
    }
    
    // Check for end of octant
    if ((abs(arcAxis[arcDominantAxis]) >= abs(arcAxis[arcFollowingAxis]) && arcOctantSign == POS)
      || (((arcDominantSign == POS) ? -arcAxis[arcDominantAxis] : arcAxis[arcDominantAxis]) <= 0 && arcOctantSign == NEG)) {
      arcOctant++;//(char)1 ^ arcDirectionXORValue;
      if (arcOctant == 8) arcOctant = 0;
      //if (arcOctant == -1) arcOctant = 7;
    }
    
    arcStep++; // Increment arc steps
    if (arcStep < current_move_ptr->max_delta) can_step = true;
  }
  
  // If all axes are where they need to be (or have all hit their limits) then the move is finished
  if ( !can_step ) {
    
    // Assume we have reached where we need to be
    current_steps[X_AXIS] = current_move_ptr->target_steps[X_AXIS];
    current_steps[Y_AXIS] = current_move_ptr->target_steps[Y_AXIS];
    current_steps[Z_AXIS] = current_move_ptr->target_steps[Z_AXIS];
    move_finished = true;
  }

  // Begin acceleration calculations
  sequence_step_number++;
  long delta_t = 0; // Must be initialised to zero! I wasted about three days not noticing this!
  
  // In acceleration phase
  if ( move_phase == MOVE_PHASE_ACCEL ) {
    
    // Calculate time from beginning of move (standstill) to now
    long t = timeOffset(sequence_step_number);
    
    // Subtract the last value for this to get the time interval for the next move
    delta_t = t - last_t;
    last_t = t;
    
    // Keep track of how many steps we have spent accelerating -
    // this will indicate when it is necessary to start decelerating
    // note it is not possible to use sequence_steps_to_accelerate because
    // we will not always reach full speed
    sequence_steps_accelerated = sequence_step_number;
    
    // Stop accelerating if we have reached the correct number of steps
    if (sequence_step_number >= sequence_steps_to_accelerate) {
      move_phase = MOVE_PHASE_CONST_VELOCITY;
    }
  }
    
  // In deceleration phase
  if ( move_phase == MOVE_PHASE_DECEL ) {
    long t = timeOffset(sequence_total_steps - sequence_step_number);
    delta_t = last_t - t;
    last_t = t;
  }
  
  // Do we have another move queued up with all axis velocities within tolerance?
  float max_delta_v = units_based_constants[MAX_DELTA_V];
  if ( have_next_move && next_move_ptr->continueMove ) {
      
    // If so, add the steps in the queued move to the total for the sequence
    // if this has not yet been done, and set flag to continue move
    // if we have already started decelerating do not do so
    if (!continue_move && move_phase != MOVE_PHASE_DECEL) { // Do not do so if already decelerating - note this could be improved
      sequence_total_steps += next_move_ptr->max_delta;
      continue_move = true;
    }
  }
  
  // Check whether to start decelerating
  if ( move_phase != MOVE_PHASE_DECEL ) {
    
    // Check whether we are as close to the end of the sequence as we are from the start
    // now or when we stopped accelerating (whichever is earlier)
    if ( sequence_step_number >= ( sequence_total_steps - sequence_steps_accelerated ) ) {
      last_t = timeOffset(sequence_total_steps - sequence_step_number);
      move_phase = MOVE_PHASE_DECEL;
    }
  }
 
  // Calculate timer load value from delta t
  timer1LoadValue = Timer1Delay( max( current_move_ptr->step_delay, delta_t ) );

  // Write the step pins low again - the time taken for the
  // preceding calculations will be more than enough to
  // ensure that there has been a long enough high pulse
  *step_output_reg[X_AXIS] &= ~step_bitmask[X_AXIS];
  *step_output_reg[Y_AXIS] &= ~step_bitmask[Y_AXIS];
  *step_output_reg[Z_AXIS] &= ~step_bitmask[Z_AXIS];

  // If finished, start next move
  if ( move_finished ) {

    // Make sure queue not being modified by
    // main routine and then advance
    if (!move_queue_lock_main) advance_move_queue();

    // If the queue is locked by the main routine, we can't hang about
    // and wait for it as this would freeze up the whole system,
    // so we just wait until the next time the interrupt routine
    // is called and try again
  }
  // Load new value into compare match register (disabling interrupts first)
  cli();
  OCR1A = timer1LoadValue;
  
  // Clear flag preventing re-entrancy
  inTimer1InterruptRoutine = false;
}

// Set up timer to make next move
// NOTE - it is very important that this is not called by
// the irq routine and the main code at the same time - therefore the locks
void advance_move_queue()
{

  // If there is a move waiting, start moving
  if (have_next_move) {
    
    // Signal that we are free to queue up another move
    have_next_move = false;

    // Rotate move queue;
    volatile Move *temp = next_move_ptr;
    next_move_ptr = current_move_ptr;
    current_move_ptr = temp;
  
    // Initialise counters
    step_counter[X_AXIS] = step_counter[Y_AXIS] = step_counter[Z_AXIS] = -(current_move_ptr->max_delta)/2;
    
    if (current_move_ptr->move_type == MOVE_LINEAR) {
      
      enable_steppers_linear();

      // Set direction pins
      for (unsigned int i = 0; i < 3; i++) {
#if INVERT_DIR == 1
        fastDigitalWrite(direction_output_reg[i], direction_bitmask[i], !current_move_ptr->direction[i]);
#else
        fastDigitalWrite(direction_output_reg[i], direction_bitmask[i], current_move_ptr->direction[i]);
#endif
      }
      
    } else { // MOVE_ARC
      enable_steppers_arc();
      
      arcOctant = current_move_ptr->startOctant;
      arcOldDominantAxis = dominantAxisTbl[arcOctant];
      arcOldFollowingSign = followingSignTbl[arcOctant];
      arcAxis[X_AXIS] = current_move_ptr->arc_startPoint[X_AXIS];
      arcAxis[Y_AXIS] = current_move_ptr->arc_startPoint[Y_AXIS];
      arcF = current_move_ptr->initial_f;
      arcStep = 0;
      
      // A value is XOR-ed with the relevant sign values to quickly invert them for counterclockwise arcs
      arcDirectionXORValue = (current_move_ptr->arc_direction == ARC_CLOCKWISE) ? ARC_XOR_CLOCKWISE : ARC_XOR_COUNTERCLOCKWISE;
    }
    
    // If we are not continuing a move
    // sequence, start a new one
    if (!continue_move) {
      move_phase = MOVE_PHASE_ACCEL;
      sequence_step_number = 1;
      sequence_total_steps = current_move_ptr->max_delta;
      sequence_steps_to_accelerate = current_move_ptr->steps_to_accelerate;
      sequence_steps_accelerated = 0;
      
      last_t = timeOffset(1); // Time for first step

      // Initial time
      timer1LoadValue = Timer1Delay( max( last_t, current_move_ptr->step_delay ) );
      
    // Otherwise reset continue move flag and continue with sequence
    } else continue_move = false;
    
    // Initialise flags and counters
    move_finished = false;
    
    // Load compare match register (must disable interrupts while doing so)
    cli();
    OCR1A = timer1LoadValue;
    sei();

    // Set Timer1 Output Compare Match interrupt enable
    TIMSK1 = 1<<OCIE1A;

    moving = true;
  } else { // Otherwise stop timer and disable steppers

    moving = false;

    // Clear Timer1 Output Compare Match interrupt enable
    TIMSK1 = 0<<OCIE1A;

    // Disable steppers - note this is not a problem with screw thread based drives,
    // but for pulley and belt systems there should really be a delay before disabling
    // steppers to allow the machine to settle, otherwise inertia will carry the
    // machine past the desired stop position

#if STEPPERS_ALWAYS_ON == 0
     disable_steppers();
#endif     
  }
}

// Generate constants to be used in acceleration calculations
void calculateAccelConstants()
{
  for (unsigned char i = 0; i < 3; i++) {
    accel_const[i] = ( 1000000000000.0 / units_based_constants[i] ) * 2 / ( units_based_constants[MAX_ACCEL] / 60 );
  }
}

// Calculate the time that it should take to reach a given step number during acceleration phase
long timeOffset(long s)
{
    return (long)(sqrt_approx(current_move_ptr->accel_const * s));
//  return (long)(sqrt(current_move_ptr->accel_const * s));
}


// Fast square root (approximation)
// written by John Carmack and appearing in Quake III source code (GPL)
//
// It seems no-one really knows how or why this works but it does
inline float sqrt_approx(float number) {
  
  // Note - these must be declared volatile or
  // else they will get optimised away and
  // the whole thing will break
  volatile long i;
  volatile float x, y;
  volatile const float f = 1.5F;

  x = number * 0.5F;
  y  = number;
  i  = * ( long * ) &y;
  i  = 0x5f3759df - ( i >> 1 ); // The magical constant of mystery
  y  = * ( float * ) &i;
  y  = y * ( f - ( x * y * y ) );
  //y  = y * ( f - ( x * y * y ) ); // Extra iteration not necessary
  return number * y;
}

// Compromise between speed and memory (could inline it but would take more space)
void fastDigitalWrite(volatile uint8_t* port, volatile uint8_t mask, boolean value) {
  if (value) *port |= mask; else *port &= ~mask;
}

// Not debounced - hope that's not going to be a problem
boolean canStep(unsigned char axis, bool direction) {
#if ENDSTOPS_MIN_ENABLED == 1
#if ENDSTOPS_INVERTING == 1
  if (!(*min_input_reg[axis] & min_bitmask[axis]) && !direction) return false;
#else
  if ((*min_input_reg[axis] & min_bitmask[axis]) && !direction) return false;
#endif
#endif
#if ENDSTOPS_MAX_ENABLED == 1
#if ENDSTOPS_INVERTING == 1
  if (!(*max_input_reg[axis] & max_bitmask[axis]) && direction) return false;
#else
  if ((*max_input_reg[axis] & max_bitmask[axis]) && direction) return false;
#endif
#endif
  return true;
}
