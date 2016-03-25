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

// LED matrix display
#define WIDTH 8
#define HEIGHT 4
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
int wifi_mode = 0; // scanning, joined, leader

#define SCAN_INTERVAL 10000
int last_scan = 0; // periodic scanning timer, ms of last scan
int wifi_scans = 0; // how many scans have we done?

#define BEACON_INTERVAL 1000
int last_beacon;


// leaders will start their network names with this prefix
String wifi_prefix = "blinky-";


void setup()
{
	WiFi.mode(WIFI_STA);
	//WiFi.begin(ssid, password);

	udp.begin(UDP_PORT);
	last_scan = millis();

	// generate a random id for ourselves
	wifi_my_id = random(99998) + 1;

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
	WiFi.softAP(ssid.c_str());
	wifi_mode = MODE_CANDIDATE;

	Serial.println("Candidate " + ssid);
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
	}

	int now = millis();
	if (now - last_beacon > BEACON_INTERVAL)
	{
		last_beacon = now;
		IPAddress bcast = WiFi.localIP();
		bcast[3] = 255;
		udp.beginPacket(bcast, UDP_PORT);
		udp.write(String(now).c_str());
		udp.endPacket();
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
	}

	int now = millis();
	if (now - last_beacon > BEACON_INTERVAL)
	{
		last_beacon = now;
		IPAddress bcast = WiFi.localIP();
		bcast[3] = 255;
		udp.beginPacket(bcast, UDP_PORT);
		udp.write(String(now).c_str());
		udp.endPacket();
	}
}


void wifi_leader()
{
	// nothing to do yet
}

void loop()
{
	switch(wifi_mode)
	{
	case MODE_SCANNING: wifi_scanning(); break;
	case MODE_CANDIDATE: wifi_candidate(); break;
	case MODE_FOLLOWER: wifi_follower(); break;
	case MODE_LEADER: wifi_leader(); break;
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
*/
	
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
}


