#include <avr/io.h>


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

	for(int i = 0 ; i < 255 ; i++)
		color(i, i, i);
	for(int i = 0 ; i < 255 ; i++)
		color(i, 0, 0);
	for(int i = 0 ; i < 255 ; i++)
		color(0, i, 0);
	for(int i = 0 ; i < 255 ; i++)
		color(0, 0, i);
}
