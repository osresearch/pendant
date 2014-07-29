#define RED_PIN 0
#define BLUE_PIN 1
#define GREEN_PIN 2

static inline void red_on(void)   { PORTB |= 1 << RED_PIN; }
static inline void green_on(void) { PORTB |= 1 << GREEN_PIN; }
static inline void blue_on(void)  { PORTB |= 1 << BLUE_PIN; }

static inline void red_off(void)   { PORTB &= ~(1 << RED_PIN); }
static inline void green_off(void) { PORTB &= ~(1 << GREEN_PIN); }
static inline void blue_off(void)  { PORTB &= ~(1 << BLUE_PIN); }

static inline void all_off(void)
{
	PORTB &= ~(0
		| ( 1 << RED_PIN )
		| ( 1 << GREEN_PIN )
		| ( 1 << BLUE_PIN )
		);
}


void color(
	uint8_t r,
	uint8_t g,
	uint8_t b
)
{
	if (r > 0) red_on();
	if (g > 0) green_on();
	if (b > 0) blue_on();

	for (unsigned i = 0 ; i < 255 ; i++)
	{
		if (r == i) red_off();
		if (g == i) green_off();
		if (b == i) blue_off();
	}

	all_off();
}

static const uint8_t palette[][4] PROGMEM =
{
#if 0
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

#if 1
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


static void
old_delay(unsigned length)
{
	while (length--)
	{
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop"); asm("nop");
	}
}


void
setup()
{
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 0);
  digitalWrite(1, 0);
  digitalWrite(2, 0);
}


void
loop()
{
  	uintptr_t from = (uintptr_t) palette[0];
	uintptr_t to = (uintptr_t) palette[1];
	const uintptr_t end = from + sizeof(palette);

	while (1)
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
			r /= 1;
			g /= 1;
			b /= 1;

			color(r, g, b);
			delay(1);
		}
if (random(20) == 0) {
  for(int j=0 ; j < 10 ; j++) color(128,128,128); 
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
}
