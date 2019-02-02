
// configurables
#define MAX_GATE_TIME 1000000 // in microseconds
#define CLOCK_LENGTH 10		  // in microseconds

// sensor inputs
#define PIN_SPEED A0
#define PIN_GATELEN A1
#define PIN_4051_Z A2
#define PIN_MAINSW A3
#define PIN_CLKINSW A6
#define PIN_CLKIN 2

#define PIN_165_CLK 7 // XX
#define PIN_165_SHLD 8
#define PIN_165_QH 9

// sensor outputs
#define PIN_CLKOUT 3
#define PIN_GATEOUT 12
#define PIN_594_SER 10
#define PIN_594_RCLK 11
#define PIN_594_SRCLK 13 // XX

// control outputs
#define PIN_4051_S0 4
#define PIN_4051_S1 5
#define PIN_4051_S2 6

#define HC165_PULSE_WIDTH_USEC 5

// Set SERIAL_DEBUG to print debug info to serial
#define SERIAL_DEBUG

void setup()
{
#ifdef SERIAL_DEBUG
	Serial.begin(115200);
#endif
	pinMode(PIN_CLKOUT, OUTPUT);
	pinMode(PIN_GATEOUT, OUTPUT);
	pinMode(PIN_4051_S0, OUTPUT);
	pinMode(PIN_4051_S1, OUTPUT);
	pinMode(PIN_4051_S2, OUTPUT);
	pinMode(PIN_594_SER, OUTPUT);
	pinMode(PIN_594_RCLK, OUTPUT);
	pinMode(PIN_594_SRCLK, OUTPUT);
	pinMode(PIN_165_CLK, OUTPUT);
	pinMode(PIN_165_SHLD, OUTPUT);
	pinMode(PIN_165_QH, INPUT);
	pinMode(PIN_CLKIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(PIN_CLKIN), clock_receive, RISING);
}

int step = 0;
int stepCount = 0;
int maxSteps = 1;
int reverse = 0;

unsigned long lastNotePlayed = micros();
bool gateOpen = false;

// clock_receive is a hardware interrupt callback.
byte clkState = LOW;
volatile byte clock = LOW;
void clock_receive()
{
	clock = !clock;
}


// myAnalogRead reads the analog input twice
// to stabilize the ADC input before returning
// a value.
int myAnalogRead(int pin)
{
	analogRead(pin);
	return analogRead(pin);
}

void loop()
{
	// gatelen goes from 0.0 to 1.0
	float gatelen = float(myAnalogRead(PIN_GATELEN)) / 1024.0;

	// calc note length in microseconds, this ranges from about 6 to 300 bpm or so
	unsigned long speed = 60000000.0 / (float(myAnalogRead(PIN_SPEED)) + 10.0);

	// clocksw is high if an external clock is plugged in.
	bool intclock = myAnalogRead(PIN_CLKINSW) < 512;

	// if the gate is still open, turn it off after some time.
	if (gateOpen && lastNotePlayed < micros() - (MAX_GATE_TIME * gatelen))
	{
		digitalWrite(PIN_GATEOUT, LOW);
		shiftOut(0);
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
		shiftOut(0);
		gateOpen = false;
	}

	lastNotePlayed = micros();
	gateOpen = do_step();
}

bool do_step()
{
	// mainsw is the 'pingpong' switch
	int mainsw = myAnalogRead(PIN_MAINSW);

	stepCount++;
	if (stepCount >= maxSteps)
	{
		stepCount = 0;

		bool pingpong = mainsw > 768;
		if (reverse && pingpong)
		{
			step--;
			if (step < 0)
			{
				step = 7;
				if (pingpong)
				{
					reverse = 0;
					step = 1;
				}
			}
		}
		else
		{
			step++;
			if (step == 8)
			{
				step = 0;
				if (pingpong)
				{
					reverse = 1;
					step = 6;
				}
			}
		}
	}

	digitalWrite(PIN_4051_S0, bitRead(step, 0));
	digitalWrite(PIN_4051_S1, bitRead(step, 1));
	digitalWrite(PIN_4051_S2, bitRead(step, 2));

	maxSteps = myAnalogRead(PIN_4051_Z) / 128;

	unsigned char switches = read_shift_reg();
	bool sw = switches & (1 << step);

	// toggle clock and gate if the gate switch is on.
	digitalWrite(PIN_CLKOUT, HIGH);
	if (sw)
	{
		digitalWrite(PIN_GATEOUT, HIGH);
		shiftOut(1 << step);
	}
	// clock time is 10us
	delayMicroseconds(CLOCK_LENGTH);
	digitalWrite(PIN_CLKOUT, LOW);

	// return wether or not we want a gate stop.
	return sw;
}

// read_shift_reg reads the 74hc165 shift register for all switch values.
unsigned char read_shift_reg()
{
	unsigned char byte = 0;

	// Trigger parallel load first
	digitalWrite(PIN_165_SHLD, LOW);
	delayMicroseconds(HC165_PULSE_WIDTH_USEC);
	digitalWrite(PIN_165_SHLD, HIGH);
	delayMicroseconds(HC165_PULSE_WIDTH_USEC);

	for (int i = 0; i < 8; i++)
	{
		int bit = digitalRead(PIN_165_QH);
		byte |= (bit << (7 - i));

		digitalWrite(PIN_165_CLK, HIGH);
		delayMicroseconds(HC165_PULSE_WIDTH_USEC);
		digitalWrite(PIN_165_CLK, LOW);
		delayMicroseconds(HC165_PULSE_WIDTH_USEC);
	}
	return (byte);
}

// shiftOut writes a byte to the hc594 shift register
// used to control the LEDs.
void shiftOut(byte data)
{
	digitalWrite(PIN_594_RCLK, LOW);
	for (int i = 7; i >= 0; i--)
	{
		digitalWrite(PIN_594_SRCLK, LOW);

		int pinState;
		if (data & (1 << i))
		{
			pinState = 1;
		}
		else
		{
			pinState = 0;
		}

		digitalWrite(PIN_594_SER, pinState);
		digitalWrite(PIN_594_SRCLK, HIGH);
		digitalWrite(PIN_594_SER, LOW);
	}
	digitalWrite(PIN_594_SRCLK, LOW);
	digitalWrite(PIN_594_RCLK, HIGH);
}
