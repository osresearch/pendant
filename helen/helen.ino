/** \file
 * HELEN board charlieplexing setup.
 *
 * This sketch uses Charlieplexing with four digital output pins
 * on the HELEN board to drive 12 LEDs with PWM.
 *
 * More info: https://trmm.net/HELEN and  https://trmm.net/Charlieplex
 */


#define NUM_LEDS 12 // Maximum with 3 output pins

#define CHARLIE(pin_vcc, pin_gnd) \
	{ 1 << (pin_vcc), 1 << (pin_gnd) }

static const uint8_t mux[NUM_LEDS][2] = {
	CHARLIE(1,2),
	CHARLIE(2,1),
	CHARLIE(0,1),
	CHARLIE(1,0),
	CHARLIE(4,1),
	CHARLIE(1,4),
	CHARLIE(0,4),
	CHARLIE(4,0),
	CHARLIE(2,0),
	CHARLIE(0,2),
	CHARLIE(4,2),
	CHARLIE(2,4),
};

void off()
{
	DDRB = 0;
	PORTB = 0;
}

void on(int n)
{
	uint8_t vcc = mux[n][0];
	uint8_t gnd = mux[n][1];
  
	// turn both to output
	PORTB = 0;
	DDRB = vcc | gnd;
	PORTB = vcc;
}

static uint8_t fb[NUM_LEDS];

void setup()
{
  off();
}


void draw()
{
	for(uint8_t i = 0 ; i < 255 ; i++)
	{
		for (uint8_t j = 0 ; j < NUM_LEDS ; j++)
		{
			if (fb[j] > i)
				on(j);
			else
				off();
		}
	}
}


static void
decay(
	const uint8_t speed
)
{
	for(int i = 0 ; i < NUM_LEDS ; i++)
	{
		if (fb[i] > speed)
			fb[i] -= speed;
		else
			fb[i] = 0;
	}
}


void chase1(void)
{
	// "smooth" the x value across the leds
	// not quite as nice as the frame buffer decay approach
	for (int x = 0 ; x < 2048 ; x += 3)
	{
		for (int i = 0 ; i < NUM_LEDS ; i++)
		{
			int dx;
			if (x > 1024 && i < NUM_LEDS/2)
			{
				dx = x - (i+NUM_LEDS) * 2048/NUM_LEDS;
			} else {
				dx = x - i * 2048/NUM_LEDS;
			}
			if (dx < 0)
				dx = -dx;
			dx /= 2;
			if (dx > 255)
				dx = 255;
			fb[i] = 255 - dx;
		}
		draw();
	}
}


void chase2()
{
	uint8_t x1 = 0;
	uint8_t x2 = 6;
	uint8_t t1 = 0;
	uint8_t t2 = 0;

	while (1)
	{
		if (fb[x1] < 200)
			fb[x1] += 16;
		if (fb[x2] < 200)
			fb[x2] += 6;

		if (t1++ > 37)
		{
			if (x1 == NUM_LEDS-1)
				x1 = 0;
			else
				x1++;
			t1 = 0;
		}

		if (t2++ > 17)
		{
			if (x2 == NUM_LEDS-1)
				x2 = 0;
			else
				x2++;
			t2 = 0;
		}

		draw();
		decay(1);
	}
}

void loop()
{
	chase2();
}
