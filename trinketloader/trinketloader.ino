// Standalone AVR ISP programmer
// August 2011 by Limor Fried / Ladyada / Adafruit
// Jan 2011 by Bill Westfield ("WestfW")
//
// this sketch allows an Arduino to program a flash program
// into any AVR if you can fit the HEX file into program memory
// No computer is necessary. Two LEDs for status notification
// Press button to program a new chip. Piezo beeper for error/success 
// This is ideal for very fast mass-programming of chips!
//
// It is based on AVRISP and uses the hardware SPI pins


#include "optiLoader.h"
#include "SPI.h"

// Global Variables
int pmode=0;
byte pageBuffer[128];		       /* One page of flash */


/*
 * Pins to target for a teensy
 */
#define SCK 1
#define MISO 3
#define MOSI 2
#define RESET 10
#define CLOCK 4     // self-generate 8mhz clock - handy, but not used?

#define BUTTON 14

void __attribute__((__noreturn__)) fatal(const char *string)
{ 
  Serial.println(string); 
  digitalWrite(LED_PROGMODE, 0);
  digitalWrite(LED_ERR, 1);
  
  while(1) {
    //tone(PIEZOPIN, 4000, 500);
  }

}


void setup () {
  Serial.begin(115200);			/* Initialize serial for status msgs */
  Serial.println("\nTrinket loader!");
}

void loop (void) {
  pinMode(BUTTON, INPUT);     // button for next programming
  digitalWrite(BUTTON, HIGH); // pullup
  delay(10);
 
  pinMode(LED_PROGMODE, OUTPUT);
  pinMode(LED_ERR, OUTPUT);
  analogWrite(LED_PROGMODE, 0);
  digitalWrite(LED_ERR, 0);

  Serial.println("\nType 'G' or hit BUTTON for next chip");
  uint8_t bright = 0;
  while (1) {
    analogWrite(LED_PROGMODE, bright < 128 ? bright : 255 - bright);
    bright++;
    if  ((! digitalRead(BUTTON)) || (Serial.read() == 'G'))
      break;  
    delay(10);
  }
  analogWrite(LED_PROGMODE, 0);

  while (!digitalRead(BUTTON)) {
    delay(10);
  }
  delay(100);
 
  target_poweron();			/* Turn on target power */

  uint16_t signature;
  const image_t *targetimage;
        
  if (! (signature = readSignature()))		// Figure out what kind of CPU
    fatal("Signature fail");
  if (! (targetimage = findImage(signature)))	// look for an image
    fatal("Image fail");
  
  eraseChip();

  if (! programFuses(targetimage->image_progfuses))	// get fuses ready to program
    fatal("Programming Fuses fail");
  
/*
  if (! verifyFuses(targetimage->image_progfuses, targetimage->fusemask) ) {
    fatal("Failed to verify fuses");
  } 
*/

  end_pmode();
  start_pmode();

  const byte *hextext = targetimage->image_hexcode;  
  uint16_t pageaddr = 0;
  uint8_t pagesize = pgm_read_byte(&targetimage->image_pagesize);
  uint16_t chipsize = pgm_read_word(&targetimage->chipsize);
        
  //Serial.println(chipsize, DEC);
  while (pageaddr < chipsize) {
     const byte *hextextpos = readImagePage (hextext, pageaddr, pagesize, pageBuffer);
          
     boolean blankpage = true;
     for (uint8_t i=0; i<pagesize; i++) {
       if (pageBuffer[i] != 0xFF) blankpage = false;
     }          
     if (! blankpage) {
       if (! flashPage(pageBuffer, pageaddr, pagesize))	
	 fatal("Flash programming failed");
     }
     hextext = hextextpos;
     pageaddr += pagesize;
  }
  
  // Set fuses to 'final' state
  if (! programFuses(targetimage->image_normfuses))
    fatal("Programming Fuses fail");
    
  end_pmode();
  start_pmode();
  
  Serial.println(F("\nVerifing flash..."));
  if (! verifyImage(targetimage->image_hexcode) ) {
    fatal("Failed to verify chip");
  } else {
    Serial.println(F("\tFlash verified correctly!"));
  }

  if (! verifyFuses(targetimage->image_normfuses, targetimage->fusemask) ) {
    fatal("Failed to verify fuses");
  } else {
    Serial.println(F("Fuses verified correctly!"));
  }
  target_poweroff();			/* turn power off */

  Serial.println("*OK!*");

  while (1)
  {
    digitalWrite(LED_PROGMODE, HIGH);
    delay(250);
    digitalWrite(LED_PROGMODE, LOW);
    delay(250);
    digitalWrite(LED_PROGMODE, HIGH);
    delay(250);
    digitalWrite(LED_PROGMODE, LOW);
    delay(500);
  }
}


void start_pmode () {
  pinMode(SCK, INPUT); // restore to default

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128); 
  
  debug("...spi_init done");
  // following delays may not work on all targets...
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);
  pinMode(SCK, OUTPUT);
  digitalWrite(SCK, LOW);
  delay(50);
  digitalWrite(RESET, LOW);
  delay(50);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  debug("...spi_transaction");
  spi_transaction(0xAC, 0x53, 0x00, 0x00);
  debug("...Done");
  pmode = 1;
}

void end_pmode () {
  SPCR = 0;				/* reset SPI */
  digitalWrite(MISO, 0);		/* Make sure pullups are off too */
  pinMode(MISO, INPUT);
  digitalWrite(MOSI, 0);
  pinMode(MOSI, INPUT);
  digitalWrite(SCK, 0);
  pinMode(SCK, INPUT);
  digitalWrite(RESET, 0);
  pinMode(RESET, INPUT);
  pmode = 0;
}


/*
 * target_poweron
 * begin programming
 */
boolean target_poweron ()
{
  pinMode(LED_PROGMODE, OUTPUT);
  digitalWrite(LED_PROGMODE, HIGH);
  digitalWrite(RESET, LOW);  // reset it right away.
  pinMode(RESET, OUTPUT);
  delay(100);
  Serial.print(F("Starting Program Mode"));
  start_pmode();
  Serial.println(" [OK]");
  return true;
}

boolean target_poweroff ()
{
  end_pmode();
  digitalWrite(LED_PROGMODE, LOW);
  return true;
}
