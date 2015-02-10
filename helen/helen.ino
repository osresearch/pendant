/** \file
 * HELEN board charlieplexing setup.
 *
 * This sketch uses Charlieplexing with four digital output pins
 * on the HELEN board to drive 12 LEDs with PWM.
 *
 * More info: https://trmm.net/HELEN and  https://trmm.net/Charlieplex
 */

#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

ISR(WDT_vect)
{
	wdt_disable();
}

/** Go into a deep sleep for 15ms */
void
deep_sleep()
{
	set_sleep_mode(SLEEP_MODE_IDLE);
	//set_sleep_mode(SLEEP_MODE_PWR_SAVE); // doesn't work

	// enables the sleep bit in the mcucr register
	// so sleep is possible. just a safety pin 
	sleep_enable();
  
	// turn off some of the things
	power_adc_disable();

	// enable the watch dog
	wdt_reset();
	wdt_enable(WDTO_15MS);
	sleep_mode();

	// we're back! disable sleep (the ISR turns off the watchdog)
	sleep_disable();

	//power_all_enable();
}



void setup()
{
	// switch the clock sel to 1/4, which should be 2 MHz.
	CLKPR = 0x80;
	CLKPR = 0x02; // clk/4
	off();
}

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


static inline void
decay(
	const uint8_t speed,
	const uint8_t min = 0
)
{
	for(uint8_t i = 0 ; i < NUM_LEDS ; i++)
	{
		if (fb[i] > min+speed)
			fb[i] -= speed;
	}
}

static inline void
decay_up(
	const uint8_t speed,
	const uint8_t min
)
{
	for(uint8_t i = 0 ; i < NUM_LEDS ; i++)
	{
		if (fb[i] < min)
			fb[i] += speed;
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


static void
chase2()
{
	uint8_t x1 = 0;
	uint8_t x2 = 6;
	uint8_t t1 = 0;
	uint8_t t2 = 0;

	while (random(10000) > 1)
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
			if (x2 == 0)
				x2 = NUM_LEDS-1;
			else
				x2--;
			t2 = 0;
		}

		draw();
		decay(1);
	}
}


static
void twinkle()
{
	while (random(10000) > 1)
	{
		if (random(200) < 2)
		{
			const uint8_t i = random(NUM_LEDS);
			if (i < NUM_LEDS)
				fb[i] = 200;
		}

		draw();
		decay(1);
	}
}

static
void soft_twinkle()
{
	const uint8_t min = 8;
	const uint8_t speed = 1;

	while (1) //random(10000) > 1)
	{
		const uint8_t chance = random(100);
		const uint8_t i = random(NUM_LEDS);
		if (chance < 1)
		{
			if (i < NUM_LEDS)
				fb[i] = 100;
		} else
		if (chance < 5)
		{
			if (i < NUM_LEDS)
				fb[i] = 1;
		}

		draw();
		decay(speed, min);
		if (random(5) == 0)
			decay_up(1, min);
		delay(2); // deep_sleep();
	}
}


static void
total_random()
{
	uint8_t max = 64;
	uint8_t dir[NUM_LEDS];
	uint8_t speed[NUM_LEDS];

	for(uint8_t i = 0 ; i < NUM_LEDS ; i++)
	{
		const uint8_t s = random(2)+1;

		fb[i] = random(max-s);
		dir[i] = 1;
		speed[i] = s;
	}

	while (random(10000) > 1)
	{
		uint16_t r = random(10000);
		for(uint8_t i = 0 ; i < NUM_LEDS ; i++)
		{
			const uint8_t b = fb[i];
			const uint8_t s = speed[i];
			r >>= 1;
			if (r & 1)
				continue;

			if (dir[i])
			{
				if (b+s > max)
				{
					dir[i] = 0;
				} else {
					fb[i] = b + s;
				}
			} else {
				if (b < s)
				{
					dir[i] = 1;
				} else {
					fb[i] = b - s;
				}
			}
		}
		draw();
		delay(15);
	}
			
}


static void
chase_smooth()
{
	const uint8_t speed = 1;
	int t = 0;

	while (++t < NUM_LEDS*64)
	{
		for(int i = 0 ; i < NUM_LEDS ; i++)
		{
			int x = i * 64;
			int dt = t - x;

			if (dt < -128 || dt > 128)
				fb[i] = 0;
			else
			if (dt < 0)
				fb[i] = 64+dt/2;
			else
				fb[i] = 64-dt/2;
		}

		draw();
		//delay(10);
	}
}


void loop()
{
if(0)
{
	chase2();
	twinkle();
	chase_smooth();
	total_random();
}
	soft_twinkle();
}
