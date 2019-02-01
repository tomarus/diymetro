// sensor inputs
#define PIN_SPEED   A0
#define PIN_GATELEN A1
#define PIN_4051_Z  A2
#define PIN_MAINSW  A3
#define PIN_CLKINSW A6
#define PIN_CLKIN   2

#define PIN_165_CLK  7
#define PIN_165_SHLD 8
#define PIN_165_QH   9

// sensor outputs
#define PIN_CLKOUT  3
#define PIN_GATEOUT 12
#define PIN_594_SER   10
#define PIN_594_RCLK  11
#define PIN_594_SRCLK 13

// control outputs
#define PIN_4051_S0 4
#define PIN_4051_S1 5
#define PIN_4051_S2 6

#define HC165_PULSE_WIDTH_USEC 5

// Set SERIAL_DEBUG to print debug info to serial
#define SERIAL_DEBUG

void setup() {
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
#endif
  pinMode(PIN_CLKOUT,    OUTPUT);
  pinMode(PIN_GATEOUT,   OUTPUT);
  pinMode(PIN_4051_S0,   OUTPUT);
  pinMode(PIN_4051_S1,   OUTPUT);
  pinMode(PIN_4051_S2,   OUTPUT);
  pinMode(PIN_594_SER,   OUTPUT);
  pinMode(PIN_594_RCLK,  OUTPUT);
  pinMode(PIN_594_SRCLK, OUTPUT);
  pinMode(PIN_165_CLK,   OUTPUT);
  pinMode(PIN_165_SHLD,  OUTPUT);
  pinMode(PIN_165_QH,    INPUT);
  pinMode(PIN_CLKIN,     INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_CLKIN), clock_receive, RISING);
}

int step = 0;
int stepCount = 0;
int maxSteps = 1;
int reverse = 0;

volatile byte clock = LOW;
void clock_receive() {
  clock = !clock;
}

void loop() {
  stepCount++; 
  if (stepCount >= maxSteps) {
    stepCount = 0;

  bool pingpong = analogRead(PIN_MAINSW) > 768;
	if (reverse && pingpong) {
		step--;
		if (step < 0) {
			step = 7;
			if (pingpong) {
				reverse = 0;
				step = 1;
			}
		}
    } else {
      step++; 
      if (step == 8) {
        step = 0;
        if (pingpong) { reverse = 1; step = 6; }
      }
    }
  }

  digitalWrite(PIN_4051_S0, bitRead(step, 0));
  digitalWrite(PIN_4051_S1, bitRead(step, 1));
  digitalWrite(PIN_4051_S2, bitRead(step, 2));
  delay(2); // FIXME
  maxSteps = analogRead(PIN_4051_Z) / 128;

  unsigned char switches = read_shift_reg();
  float gatelen = float(analogRead(PIN_GATELEN)) / 1024.0;

  int clocksw = analogRead(PIN_CLKINSW);
  clocksw = analogRead(PIN_CLKINSW);
  int mainsw = analogRead(PIN_MAINSW);
  mainsw = analogRead(PIN_MAINSW);
  
#ifdef SERIAL_DEBUG
  Serial.print("step: ");      Serial.print(step);
  Serial.print(" maxsteps: "); Serial.print(maxSteps);
  Serial.print(" speed: ");    Serial.print(analogRead(PIN_SPEED));
  Serial.print(" gatelen: ");  Serial.print(gatelen);
  Serial.print(" mainsw: ");   Serial.print(mainsw);
  Serial.print(" switches: "); Serial.print(switches, BIN);
  Serial.print(" clocksw: ");  Serial.print(clocksw);
  Serial.println("");
#endif

  float sleep = 60000.0 / (float(analogRead(PIN_SPEED)) + 10.0);
  bool sw = switches & (1 << step);
  digitalWrite(PIN_CLKOUT, HIGH);
  if (sw) {
    digitalWrite(PIN_GATEOUT, HIGH);
    shiftOut(1 << step);
  }
  delayMicroseconds(10);
  digitalWrite(PIN_CLKOUT, LOW);

  delay(analogRead(PIN_GATELEN));
//  delay(sleep * gatelen);

  if (sw) {
    digitalWrite(PIN_GATEOUT, LOW);
    shiftOut(0);
  }

  bool intclock = clocksw < 512;
  if (intclock) {
    delay(sleep * (1 - gatelen));
  } else {
    byte clkstate = clock;
    while (clkstate == clock) {
      clocksw = analogRead(PIN_CLKINSW);
      // stop waiting for ext clock if cable is removed
      if (clocksw < 512)
        break;
    }
  }
}

// read_shift_reg reads the 74hc165 shift register for all switch values.
unsigned char read_shift_reg() {
  unsigned char byte = 0;

  // Trigger parallel load first
  digitalWrite(PIN_165_SHLD, LOW);
  delayMicroseconds(HC165_PULSE_WIDTH_USEC);
  digitalWrite(PIN_165_SHLD, HIGH);
  delayMicroseconds(HC165_PULSE_WIDTH_USEC);

  for (int i = 0; i < 8; i++) {
    int bit = digitalRead(PIN_165_QH);
    byte |= (bit << (7 - i));

    digitalWrite(PIN_165_CLK, HIGH);
    delayMicroseconds(HC165_PULSE_WIDTH_USEC);
    digitalWrite(PIN_165_CLK, LOW);
    delayMicroseconds(HC165_PULSE_WIDTH_USEC);
  }
  return(byte);
}

// shiftOut writes a byte to the hc594 shift register
// used to control the LEDs.
void shiftOut(byte data) {
  digitalWrite(PIN_594_RCLK, LOW);
  for (int i = 7; i >= 0; i--)  {
    digitalWrite(PIN_594_SRCLK, LOW);

    int pinState;
    if ( data & (1 << i) ) {
      pinState = 1;
    } else {
      pinState = 0;
    }

    digitalWrite(PIN_594_SER, pinState);
    digitalWrite(PIN_594_SRCLK, HIGH);
    digitalWrite(PIN_594_SER, LOW);
  }
  digitalWrite(PIN_594_SRCLK, LOW);
  digitalWrite(PIN_594_RCLK, HIGH);
}
