/** \file
 * NeoPixel based pendant.
 * Uses a Gemma-like bootloader and a single through-hole NeoPixel.
 *
 * Due to a mistake in using a common cathode RGB LED, the
 * LED has to be powered via a data pin.
 *
 * 5mm through hole:
 * In   +5v
 *           Gnd Out
 * PB0  PB1  GND  Ignore
 *
 */
#include <Adafruit_NeoPixel.h>

#define POWER_PIN 1
#define PIXEL_PIN 0

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(
	1,
	PIXEL_PIN,
	NEO_GRB + NEO_KHZ800
);


void setup()
{
	// turn on the pixel by bringing the power pin high
	pinMode(POWER_PIN, OUTPUT);
	digitalWrite(POWER_PIN, 1);

	// Now start up the pixels
	pixel.begin();
}


void ramp(
	int r,
	int g,
	int b
)
{
	for (int i = 0 ; i < 256 ; i+=8)
	{
		pixel.setPixelColor(0, (i*r)/256, (i*g)/256, (i*b)/256);
		pixel.show();
		delay(30);
	}
	for (int i = 255 ; i >= 0 ; i-=8)
	{
		pixel.setPixelColor(0, (i*r)/256, (i*g)/256, (i*b)/256);
		pixel.show();
		delay(30);
	}
}

void loop()
{
	while(1)
	{
		ramp(32,0,0);
		//ramp(32,32,0);
		//ramp(0,16,0);
		//ramp(0,32,32);
		ramp(0,0,32);
		//ramp(32,0,32);
	}
}

#if 0
static const uint8_t palette[][4] PROGMEM =
{
#if 1
	{ 128,   0,   0 },
	{ 128, 128,   0 },
	{   0, 128,   0 },
	{   0, 128, 128 },
	{ 128, 128, 128 },
	{ 128,   0, 128 },
#endif

#if 0
	{  50,   0,   0 },
	{ 100,   0,   0 },
	{   0,  50,   0 },
	{   0, 100,   0 },
	{   0,   0,  50 },
	{   0,   0, 100 },
#endif

#if 0
	//http://www.colourlovers.com/palette/1473/Ocean_Five
	{ 0, 160, 176 },
	{ 0, 160, 176 },
	{ 0, 160, 176 },
	{ 106, 74, 60 },
	{ 106, 74, 60 },
	{ 106, 74, 60 },
	{ 204, 51, 63 },
	{ 204, 51, 63 },
	{ 204, 51, 63 },
	{ 235, 104, 65 },
	{ 235, 104, 65 },
	{ 235, 104, 65 },
	{ 237, 201, 81 },
	{ 237, 201, 81 },
	{ 237, 201, 81 },
#endif

#if 0
	// http://www.colourlovers.com/palette/3419536/Hot_Pink_Print_Short
	{ 226, 31, 128 },
	{ 73, 1, 51 },
	{ 214, 140, 204 },
	{ 227, 252, 160 },
	{ 71, 219, 253 },
#endif
};

uintptr_t from = (uintptr_t) palette[0];
uintptr_t to = (uintptr_t) palette[1];
const uintptr_t end = from + sizeof(palette);

void
loop()
{
	//while (1)
	{
		for (int i = 0 ; i < 64 ; i++)
		{
			color(i,0,0);
			delay(10);
		}
		for (int i = 0 ; i < 64 ; i++)
		{
			color(0,i,0);
			delay(10);
		}
		for (int i = 0 ; i < 64 ; i++)
		{
			color(0,0,i);
			delay(10);
		}
		for (int i = 0 ; i < 64 ; i++)
		{
			color(i,i,0);
			delay(10);
		}
	}

/*
	//while (1)
	{
		int r0 = pgm_read_byte(from + 0);
		int g0 = pgm_read_byte(from + 1);
		int b0 = pgm_read_byte(from + 2);

		int r1 = pgm_read_byte(to + 0);
		int g1 = pgm_read_byte(to + 1);
		int b1 = pgm_read_byte(to + 2);

		int rdelta = r1 - r0;
		int gdelta = g1 - g0;
		int bdelta = b1 - b0;

		for (int i = 0 ; i < 128 ; i++)
		{
			uint8_t r = r0 + (rdelta * i) / 128;
			uint8_t g = g0 + (gdelta * i) / 128;
			uint8_t b = b0 + (bdelta * i) / 128;

			// save battery -- dim the LED
			r /= 2;
			g /= 2;
			b /= 2;

			color(r, g, b);
			delay(10);
		}

		// advance our colors, wrapping if we hit the
		// end of the palette list
		from += 4;
		to += 4;

		if (from >= end)
			from = (uintptr_t) palette[0];
		if (to >= end)
			to = (uintptr_t) palette[0];
	}
*/
}
#endif
