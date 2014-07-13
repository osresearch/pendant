#include <avr/io.h>
#include <avr/pgmspace.h>

#define RED_PIN 1
#define BLUE_PIN 3
#define GREEN_PIN 4

static inline void red_off(void)   { PORTB |= 1 << RED_PIN; }
static inline void green_off(void) { PORTB |= 1 << GREEN_PIN; }
static inline void blue_off(void)  { PORTB |= 1 << BLUE_PIN; }

static inline void red_on(void)   { PORTB &= ~(1 << RED_PIN); }
static inline void green_on(void) { PORTB &= ~(1 << GREEN_PIN); }
static inline void blue_on(void)  { PORTB &= ~(1 << BLUE_PIN); }

void color(
	uint8_t r,
	uint8_t g,
	uint8_t b
)
{
	if (r > 0) red_on();
	if (g > 0) green_on();
	if (b > 0) blue_on();

	for (int i = 0 ; i < 255 ; i++)
	{
		if (r == i) red_off();
		if (g == i) green_off();
		if (b == i) blue_off();
	}

	red_off();
	blue_off();
	green_off();
}

static const uint8_t palette[][3] PROGMEM =
{
	{ 128, 0, 0 },
	{ 128, 128, 0 },
	{ 0, 128, 0 },
	{ 0, 128, 128 },
	{ 128, 128, 128 },
	{ 128, 0, 128 },
};

void
__attribute__((section(".vectors")))
main(void)
{
	// make all three output pins outputs
	DDRB |= 0
		| (1 << RED_PIN)
		| (1 << GREEN_PIN)
		| (1 << BLUE_PIN)
		;

	uintptr_t from = (uintptr_t) palette[0];
	uintptr_t to = (uintptr_t) palette[1];
	const uintptr_t end = from + sizeof(palette);

	while (1)
	{
		uint8_t r0 = pgm_read_byte(from + 0);
		uint8_t g0 = pgm_read_byte(from + 1);
		uint8_t b0 = pgm_read_byte(from + 2);

		uint8_t r1 = pgm_read_byte(to + 0);
		uint8_t g1 = pgm_read_byte(to + 1);
		uint8_t b1 = pgm_read_byte(to + 2);

		int rdelta = r1 - r0;
		int gdelta = g1 - g0;
		int bdelta = b1 - b0;

		for (uint8_t i = 0 ; i < 128 ; i++)
		{
			uint8_t r = r0 + (rdelta * i) / 128;
			uint8_t g = g0 + (gdelta * i) / 128;
			uint8_t b = b0 + (bdelta * i) / 128;
			color(r, g, b);
		}

		// advance our colors, wrapping if we hit the
		// end of the palette list
		from += 3;
		to += 3;

		if (from >= end)
			from = (uintptr_t) palette;
		if (to >= end)
			to = (uintptr_t) palette;
	}
}
