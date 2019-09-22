// Timing configurables, all values are in microseconds.
// You might need to tweak these depending on your hardware.
#define CLOCK_LENGTH 250
#define GATE_DELAY 1000

// Defines the max length in microsecs of the gate length pot.
#define MAX_GATE_TIME 1000000

// sensor inputs
#define PIN_SPEED A1
#define PIN_GATELEN A0
#define PIN_4051_Z A2
#define PIN_MAINSW A3
#define PIN_CLKINSW A4
#define PIN_GATESW A5
#define PIN_CLKIN 2
#define PIN_RSTIN 3

#define PIN_DEBUG_LED 9

// sensor outputs
#define PIN_CLKOUT 7	 // PD7
#define PIN_GATEOUT 12   // PB4
#define PIN_594_SER 10   // PB2
#define PIN_594_RCLK 11  // PB3
#define PIN_594_SRCLK 13 // PB5

// 4051 port configuration.
// You can't change this without changing PORTD logic below.
#define PIN_4051_S0 4
#define PIN_4051_S1 5
#define PIN_4051_S2 6

void setup()
{
	pinMode(PIN_CLKOUT, OUTPUT);
	pinMode(PIN_GATEOUT, OUTPUT);
	pinMode(PIN_4051_S0, OUTPUT);
	pinMode(PIN_4051_S1, OUTPUT);
	pinMode(PIN_4051_S2, OUTPUT);
	pinMode(PIN_594_SER, OUTPUT);
	pinMode(PIN_594_RCLK, OUTPUT);
	pinMode(PIN_594_SRCLK, OUTPUT);
	pinMode(PIN_DEBUG_LED, OUTPUT);
	pinMode(PIN_CLKIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(PIN_CLKIN), clock_receive, RISING);
	attachInterrupt(digitalPinToInterrupt(PIN_RSTIN), reset_receive, RISING);
	randomSeed(analogRead(0));
}

int step = 0;
int stepCount = 0;
int maxSteps = 1;
int reverse = 0;
int substep = 0;

unsigned long lastNotePlayed = micros();
bool gateOpen = false;

// clock_receive is a hardware interrupt callback.
byte clkState = LOW;
volatile byte clock = LOW;
void clock_receive()
{
	clock = !clock;
}

// reset_receive is a hardware interrupt callback.
volatile byte reset = LOW;
void reset_receive()
{
	reset = !reset;
}

// myAnalogRead reads the analog input twice
// to stabilize the ADC input before returning
// a value.
int myAnalogRead(int pin)
{
	analogRead(pin);
	delayMicroseconds(500);
	analogRead(pin);
	delayMicroseconds(500);
	return analogRead(pin);
}

void doreset()
{
	reset = false;
	stepCount = maxSteps;
	step = -1;
	reverse = 0;
	substep = -1;

	lastNotePlayed = micros();
	clkState = LOW;
	clock = HIGH;
}

void loop()
{
	if (reset)
	{
		doreset();
	}
	// gatelen goes from 0.0 to 1.0
	float gatelen = float(myAnalogRead(PIN_GATELEN)) / 1024.0;

	// calc note length in microseconds, this ranges from about 6 to 300 bpm or so
	unsigned long speed = 45000000.0 / (float(myAnalogRead(PIN_SPEED)) + 10.0);

	// clocksw is high if an external clock is plugged in.
	//	bool intclock = myAnalogRead(PIN_CLKINSW) < 512;
	int ic = myAnalogRead(PIN_CLKINSW);
	bool intclock = ic > 10 && ic < 900;
	digitalWrite(PIN_DEBUG_LED, intclock);

	// if the gate is still open, turn it off after some time.
	if (gateOpen && lastNotePlayed < micros() - (MAX_GATE_TIME * gatelen))
	{
		digitalWrite(PIN_GATEOUT, LOW);
		shiftOut2(0);
		gateOpen = false;
	}

	// decide if we want to trigger the next step of the sequencer.
	if (intclock)
	{
		// wait for certain time in internal clock mode
		if (lastNotePlayed > micros() - speed)
		{
			return;
		}
	}
	else
	{
		// wait for clock signal if ext clock is connected
		if (clkState == clock)
		{
			return;
		}
		clkState = clock;
	}

	// always close the gate before sending a new note.
	if (gateOpen)
	{
		digitalWrite(PIN_GATEOUT, LOW);
		shiftOut2(0);
		gateOpen = false;
	}

	lastNotePlayed = micros();
	gateOpen = do_step();
}

// do_step outputs clock + gate and if
// necessary advances the sequencer by a step.
bool do_step()
{
	bool advanced = false;
	stepCount++;
	if (stepCount >= maxSteps)
	{
		stepCount = 0;
		advance();
		advanced = true;
	}

	// Set PIN_4051_Sxx
	PORTD = (PORTD & 0b10001111) | (step << 4);

	// Check the value of the step switch.
	// It's either 0, 511 or 1023 (with some margin).
	int gatesw = myAnalogRead(PIN_GATESW);
	int sw = 0; // default off (middle position)
	if (gatesw > 400 && gatesw < 600)
	{
		sw = 1; // down
	}
	if (gatesw > 900)
	{
		sw = 2; // up
	}

	// Wait for this many microseconds after switching the 4051
	// before sending the gate signal.
	delayMicroseconds(GATE_DELAY);

	// toggle clock and gate if the gate switch is on.
	digitalWrite(PIN_CLKOUT, HIGH);
	bool gate = sw == 1 || (sw == 2 && advanced);
	if (gate)
	{
		digitalWrite(PIN_GATEOUT, HIGH);
		shiftOut2(1 << step);
	}
	// clock time is 10us
	delayMicroseconds(CLOCK_LENGTH);
	digitalWrite(PIN_CLKOUT, LOW);

	maxSteps = map(myAnalogRead(PIN_4051_Z), 0, 1023, 8, 1); // / 128;

	// return whether or not the gate was triggered.
	return gate;
}

// advance the sequencer in forward mode
void advance_fwd(int max)
{
	step++;
	if (step >= max)
	{
		step = 0;
	}
}

// advance the sequencer in pingpong mode
void advance_pingpong()
{
	if (reverse)
	{
		step--;
		if (step < 0)
		{
			step = 7;
			reverse = 0;
			step = 1;
		}
	}
	else
	{
		step++;
		if (step >= 8)
		{
			step = 0;
			reverse = 1;
			step = 6;
		}
	}
}

// advance the sequencer in substep mode
void advance_substep(int len)
{
	substep++;
	if (substep >= len)
	{
		substep = 0;
	}

	if (substep < len - 4)
	{
		// play notes 1-4
		step = substep % 4;
	}
	else
	{
		// play notes 5-8
		step = substep + 8 - len;
	}
}

void advance_random()
{
	step = random(8);
}

byte loop2[] = { 0, 1, 2, 3, 4, 0, 1, 2, 3, 5, 0, 1, 2, 3, 6, 0, 1, 2, 3, 7 };
void advance_loop2() {
  substep++;
  if (substep >= 20)
  {
    substep = 0;
  }
  step = loop2[substep];
}

byte rndsteps[8];
int rndstep;
void advance_random_preset_init()
{
	step = random(8);
	rndstep = 0;
	for (int i = 0; i < 8; i++)
	{
		rndsteps[i] = i;
	}
	for (int i = 0; i < 8; i++)
	{
		int x = random(7) + 1;
		byte t = rndsteps[x];
		rndsteps[x] = rndsteps[i];
		rndsteps[i] = t;
	}
}
void advance_random_preset()
{
	rndstep++;
	if (rndstep >= 8)
	{
		rndstep = 0;
	}
	step = rndsteps[rndstep];
}

int lastmainsw = 0;
// advance advances the sequencer by one step
void advance()
{
	int mainsw = map(myAnalogRead(PIN_MAINSW), 0, 1023 - (1023 / 12), 11, 0);
	bool changed = false;

	if (lastmainsw != mainsw)
	{
		lastmainsw = mainsw;
		//    step = 0;
		//    substep = 0;
		changed = true;
	}

	switch (mainsw)
	{
	case 0:
		advance_fwd(4);
		break;
	case 1:
		advance_fwd(6);
		break;
	case 2:
		advance_fwd(8);
		break;
	case 3:
		advance_pingpong();
		break;
	case 4:
		advance_substep(16);
		break;
	case 5:
		advance_substep(24);
		break;
	case 6:
		advance_substep(32);
		break;
  case 7:
    advance_loop2();
    break;
	case 8:
		advance_random();
		break;
	default:
		if (changed)
		{
			advance_random_preset_init();
		}
		advance_random_preset();
		break;
	}
}

// shiftOut writes a byte to the hc594 shift register
// used to control the LEDs.
void shiftOut2(byte data)
{
	digitalWrite(PIN_594_RCLK, LOW);
	shiftOut(PIN_594_SER, PIN_594_SRCLK, MSBFIRST, data);
	digitalWrite(PIN_594_RCLK, HIGH);
}
