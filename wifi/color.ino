/** \file
 * Colorspace functions.
 */

// This is code from Adafruit
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return leds.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return leds.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return leds.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


uint32_t rgb_dim(uint32_t rgb, int dim)
{
	if (dim < 0) {
		dim = 0;
	}

	if (dim > 256) {
		dim = 256;
	}

	int r = (rgb >> 16) & 0xFF;
	int g = (rgb >>  8) & 0xFF;
	int b = (rgb >>  0) & 0xFF;

	r = (r * dim) / 256;
	g = (g * dim) / 256;
	b = (b * dim) / 256;

	return (r << 16) | (g << 8) | (b << 0);
}
