// HeadboardLight.ino

// IRLib.h from IRLib – an Arduino library for infrared encoding and decoding
// Version 1.5   June 2014
// Copyright 2014 by Chris Young http://cyborg5.com
#include <IRLib.h>

// Streaming.h - Arduino library for supporting the << streaming operator
// Copyright (c) 2010-2012 Mikal Hart.  All rights reserved.
#include <Streaming.h>      // handy library to make debugging statements easier

// Pololu LED strip library. Available from https://github.com/pololu/pololu-led-strip-arduino
#include <PololuLedStrip.h> // library for light strip
#include "HeadboardLight.h" // include file for this project

//#define DEBUG // global debug declaration.

// Create a receiver object to listen on pin 11
IRrecv My_Receiver(9);
IRdecode My_Decoder;
unsigned long value;
unsigned long last_value;
int j;

// Create an ledStrip object on pin 12.
PololuLedStrip<12> ledStrip;
int onboard_led = 13; // just for feedback to make sure we're receiving something

// Create a buffer for holding 10 colors.  Takes 30 bytes.
#define LED_COUNT 10
rgb_color colors[LED_COUNT];
rgb_color color;
// there are two color standards used here. RGB drives the light strip, 
// but HSV is more convenient for doing color wheel progressions. There
// are functions to convert between the two.
int hsv_h = 0;
double hsv_s = 1;
double hsv_v = 0.1;
bool colorWheel = false;

hsv_color hsv_current;

// quick presets. Every other one is off for an easy way to turn everything off
rgb_color presetColor[] = 
{
	{255, 255, 255},
	{0, 0, 0},
	{16, 16, 16},
	{64, 0, 0},
	{0, 64, 0},
	{0, 0, 64},
	{64, 64, 0},
	{64, 0, 64},
	{0, 64, 64}
};
char presetColorSize = sizeof(presetColor)/3;
int presetTracker = 0;
int buzzPin = 7;
unsigned long presetTimeout = millis();	// if preset timeout maxes out, then the play button 
// will turn off (if anything is on) or turn on (if 
// anything is off).
#define PRESET_TIMEOUT 300000			// 5 minutes: 5 * 60 * 1000 (300000)

void setup()
{
#ifdef DEBUG
	Serial.begin(9600);
#endif
	My_Receiver.enableIRIn(); // Start the IR receiver
	color = hsvToRgb(hsv_h, hsv_s, hsv_v);
	writeLeds(color);
	// make sure pins are pointing the proper direction
	pinMode(onboard_led, OUTPUT);
	pinMode(buzzPin, OUTPUT);
}

void loop()
{
	//Continuously look for results. When you have them pass them to the decoder

	if ((millis() - presetTimeout) > PRESET_TIMEOUT)
	{
#ifdef DEBUG
		Serial << "(1)presetTimeout timed out. hsv_v: " << hsv_v << endl;
#endif
		if (hsv_v > 0.0)
		{
			presetTracker = 1; // off
		}
		else
		{
			presetTracker = 0; // full bright
		}
#ifdef DEBUG
		Serial << "presetTracker: " << presetTracker << endl;
#endif
	}

	if (My_Receiver.GetResults(&My_Decoder)) 
	{
		My_Decoder.decode(); //Decode the data
		if (My_Decoder.decode_type== NEC) 
		{
			value = My_Decoder.value;
#ifdef DEBUG
			Serial << "0x" << _HEX(value) << endl;
			Serial << "(2)presetTimeout: " << presetTimeout << ", millis(): " << millis() << ", PRESET_TIMEOUT: " <<  PRESET_TIMEOUT << endl;
			Serial << "colorWheel: " << colorWheel << endl;
			Serial << "hsv_v: " << hsv_v << endl;
			Serial << "last_value: " << _HEX(last_value) << endl;
#endif
#ifdef DEBUG
			Serial << "0x" << _HEX(value) << endl;
#endif
			if (value == 0xFFFFFFFF)  // holding down for repeat
			{
				value = last_value;
#ifdef DEBUG
				Serial << "repeating: " << _HEX(value) << endl;
#endif
			}
			switch(value) 
			{
			case 0x77E1D074:
			case 0x77E15054:
#ifdef DEBUG
				Serial << "up" << endl;
#endif
				colorWheel = false;
				hsv_v += 0.05;
				if (hsv_v > 0.95)
				{
					hsv_v = 1.00;
					playTone(244);
				}
				else
				{
					playTone(122);
				}
				color = hsvToRgb(hsv_h, hsv_s, hsv_v);
				break;
			case 0x77E1E074:
			case 0x77E16054:
#ifdef DEBUG
				Serial << "right" << endl;
#endif
				colorWheel = false;
				hsv_h += 10;
				if (hsv_h > 359)
				{
					hsv_h = 0;
					playTone(244);
				}
				else
				{
					playTone(200);
				}
				color = hsvToRgb(hsv_h, hsv_s, hsv_v);
				break;
			case 0x77E1B074:
			case 0x77E13054:
#ifdef DEBUG
				Serial << "down" << endl;
#endif
				colorWheel = false;
				hsv_v -= 0.05;
				if (hsv_v < 0.05)
				{
					hsv_v = 0.0;
					playTone(244);
				}
				else
				{
					playTone(122);
				}

				color = hsvToRgb(hsv_h, hsv_s, hsv_v);
				break;
			case 0x77E11074:
			case 0x77E19054:
#ifdef DEBUG
				Serial << "left" << endl;
#endif
				colorWheel = false;
				hsv_h -= 10;
				if (hsv_h < 1)
				{
					hsv_h = 359;
					playTone(244);
				}
				else
				{
					playTone(200);
				}

				color = hsvToRgb(hsv_h, hsv_s, hsv_v);
				break;
			case 0x77E12074:
			case 0x77E1A054:
#ifdef DEBUG
				Serial << "play" << endl;
#endif
				colorWheel = false;
				color = presetColor[presetTracker];
				++presetTracker;
				if (presetTracker > presetColorSize) presetTracker = 0;
				for (j = 0; j < 2; j++)
				{
					playTone(122);
					delay(20);
					playTone(244);
					delay(20);
				}

				// set current hsv values to the preset
				hsv_current = rgbToHsv(color.red, color.green, color.blue);
				hsv_h = hsv_current.h;
				hsv_s = hsv_current.s;
				hsv_v = hsv_current.v;

#ifdef DEBUG
				Serial << "hsv(a): " << hsv_h << ", " << hsv_s << ", " << hsv_v << endl << "----" << endl;
#endif

				break;
			case 0x77E14074:
			case 0x77E1C054:
#ifdef DEBUG
				Serial << "menu" << endl;
#endif
				if (colorWheel)
				{
#ifdef DEBUG
					Serial << "menu:colorwheel" << endl;
#endif
					colorWheel = false;
					for (j = 1; j <= 5; j++)
					{
						playTone(j * 20);
					}
				}
				else 
				{
#ifdef DEBUG
					Serial << "menu:!colorwheel" << endl;
#endif
					colorWheel = true;
					for (j = 5; j > 0; j--)
					{
						playTone(j * 20);
					}
				}
				break;
			}
			last_value = value;
			presetTimeout = millis();
		}
		writeLeds(color);
		My_Receiver.resume(); 		//Restart the receiver
	}

	//delay(30);

	// break into color wheel mode with current s and v
	if (colorWheel)
	{
		if (++hsv_h > 359) hsv_h = 0;
		color = hsvToRgb(hsv_h, hsv_s, hsv_v);
		writeLeds(color);
		delay(20);
	}
}

void playTone(int value)
{
	digitalWrite(onboard_led, HIGH);   // turn the LED on (HIGH is the voltage level)
	for (long i = 0; i < 64 * 3; i++ ) 
	{
		// 1 / 2048Hz = 488uS, or 244uS high and 244uS low to create 50% duty cycle
		digitalWrite(buzzPin, HIGH);
		delayMicroseconds(value);
		digitalWrite(buzzPin, LOW);
		delayMicroseconds(value);
	}
	digitalWrite(onboard_led, LOW);    // turn the LED off by making the voltage LOW
}

void writeLeds(rgb_color c)
{
	for(int i = 0; i < LED_COUNT; i++)
	{
		colors[i].red = c.red;
		colors[i].green = c.green;
		colors[i].blue = c.blue;
	}

	ledStrip.write(colors, LED_COUNT);
}

// Convert HSV to corresponding RGB
rgb_color hsvToRgb(int h, double s, double v)
{
	rgb_color rgb;

	// Make sure our arguments stay in-range
	h = max(0, min(360, h));
	s = max(0, min(1.0, s));
	v = max(0, min(1.0, v));
	if(s == 0)
	{
		// Achromatic (grey)
		rgb.red = rgb.green = rgb.blue = round(v * 255);
		return rgb;
	}
	double hs = h / 60.0; // sector 0 to 5
	int i = floor(hs);
	double f = hs - i; // factorial part of h
	double p = v * (1 - s);
	double q = v * (1 - s * f);
	double t = v * (1 - s * (1 - f));
	double r, g, b;
	switch(i)
	{
	case 0:
		r = v;
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = v;
		b = p;
		break;
	case 2:
		r = p;
		g = v;
		b = t;
		break;
	case 3:
		r = p;
		g = q;
		b = v;
		break;
	case 4:
		r = t;
		g = p;
		b = v;
		break;
	default: // case 5:
		r = v;
		g = p;
		b = q;
	}
	rgb.red = round(r * 255.0);
	rgb.green = round(g * 255.0);
	rgb.blue = round(b * 255.0);
	return rgb;
}

// convert RGB to the corresponding HSV
hsv_color rgbToHsv(unsigned char r, unsigned char g, unsigned char b) 
{
	double rd = (double) r/255;
	double gd = (double) g/255;
	double bd = (double) b/255;
	double d_h;

	double max = max(rd, max(gd, bd)), min = min(rd, min(gd, bd));
	hsv_color hsv;
	hsv.v = max;

	double d = max - min;
	hsv.s = max == 0 ? 0 : d / max;

	if (max == min) { 
		d_h = 0;
	} else {
		if (max == rd) {
			d_h = (gd - bd) / d + (gd < bd ? 6 : 0);
		} else if (max == gd) {
			d_h = (bd - rd) / d + 2;
		} else if (max == bd) {
			d_h = (rd - gd) / d + 4;
		}
		d_h /= 6;
	}
	hsv.h = d_h * 360;
	return hsv;
}