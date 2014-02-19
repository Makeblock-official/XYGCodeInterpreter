// Various routines from the Arduino sources included here to save
// space - this way it is not necessary to include code for any unused
// functions and we can do such things as hard code the serial baud rate to 19200
//
// All this saves about 1K in the final binary which is a vast amount when there's only 16K available

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void println(void)
{
  print('\r');
  print('\n');  
}

void print(char c)
{
//  RR_serialWrite(c);
  Serial.write(c);
}

void print(const char c[])
{
  while (*c)
    print(*c++);
}

void println(const char c[])
{
  print(c);
  println();
}

void println(int n)
{
  printNumber((unsigned int)n);
  println();
}

/*

void println(const char c[])
{
  print(c);
  println();
}

void print(int n)
{
  print((long) n);
}

void print(unsigned int n)
{
  print((unsigned long) n);
}

void print(long n)
{
  if (n < 0) {
    print('-');
    n = -n;
  }
  printNumber(n);
}

void print(unsigned long n)
{
  printNumber(n);
}

void println(int n)
{
  print(n);
  println();
}

void println(unsigned int n)
{
  print(n);
  println();
}

void println(long n)
{
  print(n);
  println();  
}

void println(unsigned long n)
{
  print(n);
  println();  
}

// Optimised for base 10 since that's all we're dealing with
void printNumber(unsigned long n)
{
  unsigned char buf[10]; // Max unsigned long is 4294967295 so max digits is 10
  unsigned int i = 0;

  if (n == 0) {
    print('0');
    return;
  } 

  while (n > 0) {
    buf[i++] = n % 10;
    n /= 10;
  }

  for (; i > 0; i--)
    print((char)('0' + buf[i - 1]));
}

*/
// Optimised for unsigned int base 10 since that's all we're dealing with
void printNumber(unsigned int n)
{
  unsigned char buf[5]; // Max unsigned int is 64435 so max digits is 5
  unsigned int i = 0;

  if (n == 0) {
    print('0');
    return;
  } 

  while (n > 0) {
    buf[i++] = n % 10;
    n /= 10;
  }

  for (; i > 0; i--)
    print((char)('0' + buf[i - 1]));
}
/*
// Smaller RX buffer (is 128 in official Arduino code)
#define RX_BUFFER_SIZE 64

unsigned char rx_buffer[RX_BUFFER_SIZE];

unsigned char rx_buffer_head = 0;
unsigned char rx_buffer_tail = 0;

void RR_setupSerial()
{
  int baud = 19200; // FIXED - this means all the calculations below will be optimised to constant integers
#if defined(__AVR_ATmega168__)
	UBRR0H = ((F_CPU / 16 + baud / 2) / baud - 1) >> 8;
	UBRR0L = ((F_CPU / 16 + baud / 2) / baud - 1);
	
	// enable rx and tx
	sbi(UCSR0B, RXEN0);
	sbi(UCSR0B, TXEN0);
	
	// enable interrupt on complete reception of a byte
	sbi(UCSR0B, RXCIE0);
#else
	UBRRH = ((F_CPU / 16 + baud / 2) / baud - 1) >> 8;
	UBRRL = ((F_CPU / 16 + baud / 2) / baud - 1);
	
	// enable rx and tx
	sbi(UCSRB, RXEN);
	sbi(UCSRB, TXEN);
	
	// enable interrupt on complete reception of a byte
	sbi(UCSRB, RXCIE);
#endif
	
	// defaults to 8-bit, no parity, 1 stop bit
}

// Exactly the same as Arduino official version
void RR_serialWrite(unsigned char c)
{
#if defined(__AVR_ATmega168__)
  while (!(UCSR0A & (1 << UDRE0)))
  ;
  
  UDR0 = c;
#else
  while (!(UCSRA & (1 << UDRE)))
  ;
  
  UDR = c;
#endif
}

// Uses a compare and subtract instead of a modulus - faster and smaller
int RR_serialAvailable()
{
  int available = (RX_BUFFER_SIZE + rx_buffer_head - rx_buffer_tail);
  return (available >= RX_BUFFER_SIZE) ? available - RX_BUFFER_SIZE : available;
}

// As above, uses a compare and subtract instead of a modulus
int RR_serialRead()
{
  // if the head isn't ahead of the tail, we don't have any characters
  if (rx_buffer_head == rx_buffer_tail) {
    return -1;
  } else {
    unsigned char c = rx_buffer[rx_buffer_tail];
    rx_buffer_tail = (rx_buffer_tail + 1);
    if (rx_buffer_tail >= RX_BUFFER_SIZE) rx_buffer_tail -= RX_BUFFER_SIZE;
    return c;
  }
}


// Same as official Arduino version except doesn't check for invalid pin
void RR_pinMode(uint8_t pin, uint8_t mode) {
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  volatile uint8_t *reg;

  // JWS: can I let the optimizer do this?
  reg = portModeRegister(port);

  if (mode == INPUT) *reg &= ~bit;
  else *reg |= bit;
}

uint8_t analog_reference = DEFAULT;

// Exactly the same as the offical Arduino version
int RR_analogRead(uint8_t pin)
{
  uint8_t low, high, ch = analogInPinToBit(pin);

  // set the analog reference (high two bits of ADMUX) and select the
  // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
  // to 0 (the default).
  ADMUX = (analog_reference << 6) | (pin & 0x0f);

  // without a delay, we seem to read from the wrong channel
  //delay(1);

  // start the conversion
  sbi(ADCSRA, ADSC);

  // ADSC is cleared when the conversion finishes
  while (bit_is_set(ADCSRA, ADSC));

  // we have to read ADCL first; doing so locks both ADCL
  // and ADCH until ADCH is read.  reading ADCL second would
  // cause the results of each conversion to be discarded,

  // as ADCL and ADCH would be locked when it completed.
  low = ADCL;
  high = ADCH;

  // combine the two bytes
  return (high << 8) | low;
}

// Same as offical Arduino version except no check is made
// for pins using Timer1 because this is being used
// as the stepper timer
void RR_analogWrite(uint8_t pin, int val)
{
  // We need to make sure the PWM output is enabled for those pins
  // that support it, as we turn it off when digitally reading or
  // writing with them.  Also, make sure the pin is in output mode
  // for consistenty with Wiring, which doesn't require a pinMode
  // call for the analog output pins.
  RR_pinMode(pin, OUTPUT);
	
 // NOTE - we can't use Timer1 because it is being used
 // for the stepper timer so no point checking for it here
#if defined(__AVR_ATmega168__)
  if (digitalPinToTimer(pin) == TIMER0A) {
    // connect pwm to pin on timer 0, channel A
    sbi(TCCR0A, COM0A1);
    // set pwm duty
    OCR0A = val;
  } else if (digitalPinToTimer(pin) == TIMER0B) {
    // connect pwm to pin on timer 0, channel B
    sbi(TCCR0A, COM0B1);
    // set pwm duty
    OCR0B = val;
  } else if (digitalPinToTimer(pin) == TIMER2A) {
    // connect pwm to pin on timer 2, channel A
    sbi(TCCR2A, COM2A1);
    // set pwm duty
    OCR2A = val;	
  } else if (digitalPinToTimer(pin) == TIMER2B) {
    // connect pwm to pin on timer 2, channel B
    sbi(TCCR2A, COM2B1);
    // set pwm duty
    OCR2B = val;
  }
#else
  if (digitalPinToTimer(pin) == TIMER2) {
    // connect pwm to pin on timer 2, channel B
    sbi(TCCR2, COM21);
    // set pwm duty
    OCR2 = val;
  }
#endif
}

/*
// Again test and subtract instead of modulus
#if defined(__AVR_ATmega168__)
SIGNAL(SIG_USART_RECV)
#else
SIGNAL(SIG_UART_RECV)
#endif
{
#if defined(__AVR_ATmega168__)
  unsigned char c = UDR0;
#else
  unsigned char c = UDR;
#endif

  int i  = (rx_buffer_head + 1);
  if (i >= RX_BUFFER_SIZE) i -= RX_BUFFER_SIZE;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  if (i != rx_buffer_tail) {
    rx_buffer[rx_buffer_head] = c;
    rx_buffer_head = i;
  }
}
*/

