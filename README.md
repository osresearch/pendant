
Bootloader
===

The ATtiny85 needs to be flashed with a bootloader before it can
be programmed with the Arduino IDE.  This requires extra hardware,
such as a teensy or arduino configured as an ISP.

The bootloader is then flashed via an SOIC socket.  It is possible to
via a chip clip, but this seems to be somewhat flakey due to the
charlieplexing matrix.

Using the arduino IDE, select "`File`-`Examples`-`11. Arduino ISP`"
and set the "`Tools`-`Board`" for whatever type of arduino or teensy you 
are using.  For a teensy the minimum pinout is:

	Teensy	SOIC	Use
	Vcc	8
	GND	4
	1	7	SCK
	2	5	MOSI
	3	6	MISO
	10	1	Reset

Clone the bootloader tree:

    git clone https://github.com/osresearch/gemma-bootloader

You might have to edit the `gemma-bootloader/Makefile` to have the
right path to your Arduino build environment and `avrdude` tool.  You will
likely have to edit it to set the serial device that your Teensy or
Arduino shows up as.

Connect the ArduinoISP to the ATtiny85 and flash the bootloader:

    make -C gemma-bootloader flash_lv

This should result in an ok message.  if it doesn't, check the connections
and try again.  If it is succesful you won't be able to reflash it,
since the reset pin is disabled.

