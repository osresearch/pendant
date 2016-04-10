/** \file
 * WiFi LED pendant with games.
 *
 * nc won't send to broadcast UDP addresses.  Instead use socat:
 *
 * socat - UDP4-DATAGRAM:192.168.1.255:9999,broadcast
 *
 * Start in station mode and periodically scan for networks named
 * "leader-XYZ".  If one is available, join it and contact the server.
 * If none is available, wait some period of time and then create one.
 * 
 * WiFi docs: http://www.arduino.cc/en/Reference/WiFi
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
//#include <Adafruit_ADXL345_U.h>
#include "color.h"

// LED matrix display
#define WIDTH 8
#define HEIGHT 4
#define NUM_PIXELS (WIDTH * HEIGHT)
#define LED_PIN 15

Adafruit_NeoPixel leds = Adafruit_NeoPixel(WIDTH*HEIGHT, LED_PIN, NEO_GRB);
float t = 0;
int status_led;

int mode;

float fb[WIDTH][HEIGHT][3];

// i2c connected accelerometer
//Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Network config
#define UDP_PORT 9999
WiFiUDP udp;

#define MODE_SCANNING 0
#define MODE_FOLLOWER 1
#define MODE_CANDIDATE 2
#define MODE_LEADER 3

int wifi_my_id;
uint32_t my_color;
int wifi_mode = 0; // scanning, joined, leader

#define SCAN_INTERVAL 10000
int last_scan = 0; // periodic scanning timer, ms of last scan
int wifi_scans = 0; // how many scans have we done?

#define BEACON_INTERVAL 1000
int last_beacon;


// leaders will start their network names with this prefix
String wifi_prefix = "blinky-";


// Network packet for beacons
#define MAGIC 0xdecafbad

// The beacon type
typedef struct
{
	uint32_t magic;
	uint32_t id;
	uint32_t color;
} beacon_t;

typedef struct
{
	uint32_t leader_id;
	uint32_t leader_color;
	int leader_beacon_ms;	
} follower_state_t;

follower_state_t follower_state;

typedef struct
{
	uint32_t follower_id;
	uint32_t follower_color;
	int follower_beacon_ms;	
} followling_state_t;

#define MAX_FOLLOWLINGS 16
followling_state_t followlings[MAX_FOLLOWLINGS];

void setup()
{
	WiFi.mode(WIFI_STA);
	//WiFi.begin(ssid, password);

	udp.begin(UDP_PORT);
	last_scan = millis();

	// generate a random id for ourselves
	wifi_my_id = random(99998) + 1;
	my_color = Wheel(random(256));

	leds.begin();
	leds.setBrightness(32);

	//rc = accel.begin();


	// setup the serial port and also let the RTOS print
	// debugging information to the serial port
	Serial.begin(115200);
	if(0) Serial.setDebugOutput(true);
}

uint8_t buf[128];
const float smooth = 0;
float ax, ay, az;

// TODO: have this take a pointer to a struct that is formed from the
// incoming beacon, and use that to create the new followling entry
// if the followling is not already in the array.
followling_state_t* find_followling(uint32_t follower_id)
{
	// if the followling is already in the array, return a ref to it
	for (int i = 0; i < MAX_FOLLOWLINGS; i++)
	{
		followling_state_t* f = &followlings[i];
		if ( follower_id == f->follower_id ) return f;
	}

	uint32_t now = millis();
	// we have a new followling, find it a slot in the array
	for (int i = 0; i < MAX_FOLLOWLINGS; i++)
	{
		followling_state_t* f = &followlings[i];
		if (now - f->follower_beacon_ms < 60000) continue;	

		// f is a free space.  Replace the element with the current followling	
		f->follower_id = follower_id;
		Serial.print("Adding follower: ");
		Serial.print(f->follower_id);
		Serial.print(" at index: ");
		Serial.print(i);
		return f;
	}
	
	// No free spaces!!!
	return NULL;
}

/*
 * Parse a follower packet, intended to be used by leaders and candidates
 */
boolean parse_follower_packet(const beacon_t* beacon)
{
	// Not one of ours.
	if (beacon->magic != MAGIC)
	{
		Serial.println("unable to parse packet");
		return false;
	} 

	Serial.println("follower: ");
	Serial.println(beacon->id);
	
	// First, is this a new followling?
	followling_state_t* f = find_followling(beacon->id);

	// There was no space to put the new followling
	if (!f)
	{
		Serial.println("No free space in array for new followling");
		return false;
	}

	// Finish setting the data for the followling
	f->follower_color = beacon->color;
	f->follower_beacon_ms = millis();
	return true;
}

/*
 * Scan the networks, looking for one that matches our prefix.
 *
 * Returns 0 if it is not time to scan again, -1 if there are no leaders,
 * otherwise the id of the lowest leader number seen.
 */
int rescan_network()
{
	const int now = millis();
	if (now - last_scan < SCAN_INTERVAL)
		return 0;
	last_scan = now;

	Serial.print("mode: ");
	Serial.print(wifi_mode);
	Serial.print(" scans: ");
	Serial.println(wifi_scans++);

	int min_leader = -1;
	int num = WiFi.scanNetworks();
	if (num < 0)
	{
		Serial.println("wifi scan failed?");
		return -1;
	}

	for(int i = 0 ; i < num ; i++)
	{
		Serial.print(i);
		Serial.print(' ');
		String ssid = WiFi.SSID(i);
		Serial.print(ssid);
		Serial.print(' ');
		Serial.print(WiFi.RSSI(i));

		// does the SSID match our pattern?
		if (ssid.startsWith(wifi_prefix))
		{
			// extract out the integer at the end
			String idstr = ssid.substring(wifi_prefix.length());
			int leader_id = idstr.toInt();
			Serial.print(" leader ");
			Serial.print(leader_id);
			if (min_leader > leader_id || min_leader == -1)
				min_leader = leader_id;
		}

		Serial.println();
	}

	if (min_leader < 0)
	{
		Serial.println("!!! No leaders found");
	} else {
		Serial.print("Possible leader: ");
		Serial.println(min_leader);
	}

	return min_leader;
}

/*
 * Pixel display patterns for the various modes
 */
// variables used by the color patterns
int brightness = 0;
int direction = 1;
int dim_levels[] = {2, 4, 8, 16, 32, 64, 128};
int position = 0;
void scanning_pattern() {
	if (random(0) == 0) {
		// turn them all off
		for (int i = 0; i < NUM_PIXELS; i++)
			leds.setPixelColor(i, 0, 0, 0);	

		// color the pixels
		if (direction == 1) {
			for (int i = 0; i < 7; i++)
				leds.setPixelColor(position+i, rgb_dim(my_color, dim_levels[i]));
		} else if (direction == -1) {
			for (int i = 7-1; i > 0; i--) 
				leds.setPixelColor(position-i, rgb_dim(my_color, dim_levels[i]));
		}
		leds.show();
		
		// change direction as necessary
		if (position == NUM_PIXELS) {
			direction = -1;
		} else if (position == 0) {
			direction = 1;
		}	
		position = position + direction;
		delay(20);	
	}
}

void candidate_pattern()
{
	int brightness = 256 - (millis() - last_beacon) / 4;
	if (brightness < 0)
		brightness = 0;
	int color = rgb_dim(my_color, brightness);

	for (int i = 0; i < NUM_PIXELS; i++) {
		leds.setPixelColor(i, color);
	}
	leds.show();
}

void follower_pattern() {
	uint32_t leader_color = follower_state.leader_color;
	int beacon_age = millis() - follower_state.leader_beacon_ms;
	
	if (beacon_age > 1000) {
		leader_color = 0;
	} else {
		leader_color = rgb_dim(leader_color, 256 - (beacon_age/4));	
	}

	for (int i = 0; i < NUM_PIXELS; i++) {
		leds.setPixelColor(i, rgb_dim(my_color, brightness));
	}

	// include the my leader's color from the beacon
	for(int i = 0 ; i < NUM_PIXELS ; i += 4)
		leds.setPixelColor(i, leader_color);
	leds.show();

	if (brightness == 255) {
		direction = -1;
	} else if (brightness == 0) {
		direction = 1;
	}
	brightness = brightness + direction;
}

void leader_pattern() {
	// solid self color
	int self_color = rgb_dim(my_color, 255);
	leds.setPixelColor(0, self_color);
	leds.setPixelColor(1, self_color);

	uint32_t now = millis();
	int next_led = 2; // next position, or next index in the pixel array
	// followling colors
	for (int i = 0; i < MAX_FOLLOWLINGS; i++) 
	{
		followling_state_t* f = &followlings[i];
		// we have a followling in this space
		if (f->follower_id) 
		{
			int f_brightness = 256 - (now - f->follower_beacon_ms) / 4;
			if (f_brightness < 0)
				f_brightness = 0;
			int f_color = rgb_dim(f->follower_color, f_brightness);
			leds.setPixelColor(i+2, f_color);
			next_led++;
		}
	}

	// Self color tht puslses with beacons sent
	int s_brightness = 256 - (now - last_beacon) / 4;
	if (s_brightness < 0)
		s_brightness = 0;
	int s_color = rgb_dim(my_color, s_brightness);

	for (int i = next_led; i < NUM_PIXELS; i++) {
		leds.setPixelColor(i, s_color);
	}
	leds.show();
}

/*
 * Wifi protocol code for the various modes
 */
void wifi_follow(int leader_id)
{
	WiFi.mode(WIFI_STA);
	String ssid = wifi_prefix + String(leader_id);
	int rc = WiFi.begin(ssid.c_str());

	if (rc == WL_CONNECTED)
	{
		wifi_mode = MODE_FOLLOWER;
		Serial.print("Followed ");
		Serial.print(ssid);
		Serial.print(": ");
		Serial.println(WiFi.localIP());
	
	} else {
		wifi_mode = MODE_SCANNING;
		Serial.print("ERROR FOLLOWING: ");
		Serial.println(ssid);
	}
}


void wifi_create(int leader_id)
{
	WiFi.mode(WIFI_AP_STA);
	String ssid = wifi_prefix + String(leader_id);
	IPAddress ip(192,168,4,1);
	IPAddress gw(192,168,4,1);
	IPAddress subnet(255,255,255,0);
	WiFi.softAP(ssid.c_str());
	WiFi.config(ip, gw, subnet);
	wifi_mode = MODE_CANDIDATE;

	Serial.println("Candidate " + ssid + " " + WiFi.localIP());
}


void wifi_scanning()
{
	int min_leader = rescan_network();
	if (min_leader == 0)
		return;

	// if we found a new leader! join it
	if (min_leader > 0)
	{
		wifi_follow(min_leader);
		return;
	}

	// should we elect ourselves as a candidate?
	if (random(10) < wifi_scans)
	{
		wifi_scans = 0;
		wifi_create(wifi_my_id);
		return;
	}
}

void wifi_candidate()
{
	int min_leader = rescan_network();
	if (min_leader < 0)
		return;

	// there is a new leader out there, but don't follow if it
	// has an id higher than ours.
	if (min_leader != 0 && min_leader < wifi_my_id)
	{
		wifi_follow(min_leader);
		return;
	}

	// check to see if we have any followers
	int len = udp.parsePacket();
	if (len)
	{
		IPAddress ip = udp.remoteIP();
		Serial.print(ip);
		Serial.print(' ');
		Serial.print(len);
		udp.read(buf, sizeof(buf));
		for(int i = 0 ; i < len ; i++)
		{
			Serial.print(' ');
			Serial.print(buf[i], HEX);
		}

		Serial.println();

		// Parse this packet to see if it's a legit follower
		const beacon_t * beacon = (const beacon_t*) buf;
		// If we successfully took on a follower, transition to leader
		if (parse_follower_packet(beacon))
		{
			Serial.println("going to leader mode");
			wifi_mode = MODE_LEADER;
		}
	}

	// Send a beacon to potential followers/leaders
	int now = millis();
	if (now - last_beacon > BEACON_INTERVAL)
	{
		beacon_t beacon = {
			MAGIC,
			wifi_my_id,
			my_color
		};

		last_beacon = now;
		IPAddress bcast = WiFi.localIP();
		bcast[3] = 255;
		udp.beginPacket(bcast, UDP_PORT);
		udp.write((const uint8_t*) &beacon, sizeof(beacon));
		udp.endPacket();
	 	Serial.println("sent beacon");
	}
}

void wifi_follower()
{
	// did we get a packet from the leader?
	int len = udp.parsePacket();
	if (len)
	{
		IPAddress ip = udp.remoteIP();
		Serial.print(ip);
		Serial.print(' ');
		Serial.print(len);
		udp.read(buf, sizeof(buf));
		for(int i = 0 ; i < len ; i++)
		{
			Serial.print(' ');
			Serial.print(buf[i], HEX);
		}

		Serial.println();

		// parse the leader's packet
		const beacon_t * beacon = (const beacon_t*) buf;
		if (beacon->magic != MAGIC)
		{
			Serial.println("unable to parse packet");
		} else {
			Serial.print("leader: ");
			Serial.println(beacon->id);

			// Update follower state from beacon
			follower_state.leader_id = beacon->id;
			follower_state.leader_color = beacon->color;
			follower_state.leader_beacon_ms = millis();
		}
	}

	int now = millis();
	if (now - last_beacon > BEACON_INTERVAL)
	{
		beacon_t beacon = {
			MAGIC,
			wifi_my_id,
			my_color
		};

		last_beacon = now;
		IPAddress leader = WiFi.localIP();
		leader[3] = 1; // assume that the leader is also 192.168.x.1
		udp.beginPacket(leader, UDP_PORT);
		udp.write((const uint8_t *) &beacon, sizeof(beacon));
		udp.endPacket();
	}
}


void wifi_leader()
{
	// did we get a packet from a follower?
	int len = udp.parsePacket();
	if (len)
	{
		IPAddress ip = udp.remoteIP();
		Serial.print(ip);
		Serial.print(' ');
		Serial.print(len);
		udp.read(buf, sizeof(buf));
		for(int i = 0 ; i < len ; i++)
		{
			Serial.print(' ');
			Serial.print(buf[i], HEX);
		}

		Serial.println();

		// parse the follower's packet
		const beacon_t * beacon = (const beacon_t*) buf;
		parse_follower_packet(beacon);
	}

	// Send out our own beacon
	int now = millis();
	if (now - last_beacon > BEACON_INTERVAL)
	{
		beacon_t beacon = {
			MAGIC,
			wifi_my_id,
			my_color
		};

		last_beacon = now;
		IPAddress bcast = WiFi.localIP();
		bcast[3] = 255; 
		udp.beginPacket(bcast, UDP_PORT);
		udp.write((const uint8_t *) &beacon, sizeof(beacon));
		udp.endPacket();
	}
}

// This is code was adapted from code from Adafruit
// Makes the rainbow equally distributed throughout
void rainbowCycle() {
  uint16_t i, j;
  int bright = 0;

  for(j=0; j<256*5; j++, bright = (bright + 1) % 4096) { // 5 cycles of all colors on wheel
    for(i=0; i< leds.numPixels(); i++) {
      uint32_t color = Wheel(((i * 256 / leds.numPixels()) + j) & 255);
      leds.setPixelColor(i, rgb_dim(color, bright / (4096 / 256)));
    }
    leds.show();
  }
}

void loop()
{
	switch(wifi_mode)
	{
		case MODE_SCANNING: {
			scanning_pattern();
			wifi_scanning();
			break;
		}
		case MODE_CANDIDATE: {
			candidate_pattern();
			wifi_candidate(); 
			break;
		}
		case MODE_FOLLOWER: {
			wifi_follower();
			follower_pattern();
			break;
		}
		case MODE_LEADER: {
			wifi_leader(); 
			leader_pattern();
			break;
		}
		default:
			wifi_mode = MODE_SCANNING;
	}


/*
	Serial.print(accel.read16(ADXL345_REG_DATAX0));
	Serial.print(' ');
	Serial.print(accel.read16(ADXL345_REG_DATAY0));
	Serial.print(' ');
	Serial.println(accel.read16(ADXL345_REG_DATAZ0));
	//Serial.println(accel.getDeviceID());
	sensors_event_t ev;  
	accel.getEvent(&ev);
	ax = (ax * smooth + ev.acceleration.x) / (smooth+1);
	ay = (ay * smooth + ev.acceleration.y) / (smooth+1);
	az = (az * smooth + ev.acceleration.z) / (smooth+1);
	
	Serial.print(rc);
	Serial.print(' ');
	Serial.print(ax);
	Serial.print(' ');
	Serial.print(ay);
	Serial.print(' ');
	Serial.print(az);
	Serial.println();
	Serial.print(accel.getX());
	Serial.print(' ');
	Serial.print(accel.getY());
	Serial.print(' ');
	Serial.print(accel.getZ());
	Serial.println();
	
	// circling status LED
	{
		boolean connected = WiFi.status() == WL_CONNECTED;

		int status_x, status_y;
		if (status_led < WIDTH)
		{
			status_x = status_led - 0;
			status_y = 0;
		} else
		if (status_led < WIDTH + HEIGHT)
		{
			status_x = WIDTH-1;
			status_y = status_led - WIDTH;
		} else
		if (status_led < 2 * WIDTH + HEIGHT)
		{
			status_x = 2 * WIDTH + HEIGHT - status_led - 1;
			status_y = HEIGHT-1;
		} else
		if (status_led < 2 * WIDTH + 2 * HEIGHT)
		{
			status_x = 0;
			status_y = 2 * WIDTH + 2 * HEIGHT - status_led - 1;
		}

		static unsigned f;
		if (f++ % 3 == 0)
			status_led = (status_led + 1) % (2*WIDTH + 2*HEIGHT);

		if (connected)
			fb[status_x][status_y][2] = 255;
		else
			fb[status_x][status_y][1] = 255;
	}


	for(int x = 0 ; x < WIDTH ; x++)
	{
		for(int y = 0 ; y < HEIGHT ; y++)
		{
			float *p = fb[x][y];

			// pulse red up and down
			p[0] += 128 * sin(t + x * 0.1 + y * 0.2);

			// fade and clamp it
			for(int i = 0 ; i < 3; i++)
			{
				float c = p[i] * 0.9;
				p[i] = c < 0 ? 0 : c > 255 ? 255 : c;
			}

			leds.setPixelColor(x+y*WIDTH, p[0], p[1], p[2]);
		}
	}

	t += 0.02;
	leds.show();
	delay(10);
*/
}

