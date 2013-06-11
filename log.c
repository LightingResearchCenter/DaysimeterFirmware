#include "msp430.h"
#include "log.h"
#include "ADXL.h"
#include "eeprom.h"
#include "opticalsensor.h"
#include "timers.h"
#include "i2c.h"
//#include "AndyMscFseData32kEEPROM.h"

int measCount = 0;

void log() {
	//eepromGetStatus();
    switch(eepromStatus) {

    //this case doesn't do anything at this point
    case 48:
    	wdReset_1000();
    	//tbDelayMode(10,3); 		//10 ms delay, LPM3
    	break;

    //this is the intermediate state that happens on startup
    case 50:

    	tbDelayMode(10,3); 		//10 ms delay, LPM3
    	//clear memory
    	eepromReset();
    	//move to log mode
    	eepromSetStatus(0x34);
    	tbDelayMode(10,3); 		//10 ms delay, LPM3
    	//setup everything else for logging
    	eepromAddress = 1024; //16;
    	measCount = 0;
    	setupADXL();
    	ADXLreset();
    	RGBreset();
    	eepromGetPeriod();
    	//eepromPeriod /= 15; 	// REMOVE THIS AFTER TESTING!!!
    	break;

    case 52:

    	//break out and go to standby when out of memory
    	if(eepromAddress > MAX_RECORD_ADDRESS) {
    		eepromSetStatus(0x30);
    	}
    	else {

    		//read optical sensor and accelerometer every second and keep track of the number of seconds
    		readRGB();
    		readADXL();
    		measCount++;

    		//after 30 seconds, new activity
    		if((measCount % 30) == 0) {
    		//if((measCount % 2) == 0) { 		// REMOVE THIS AFTER TESTING AND UNCOMMENT ABOVE LINE!!!
    			findActivityArray();
    			ADXLreset();
    		}

    		//after one record period, store values and start a new record
    		if(measCount == eepromPeriod) {
    			findActivity();
    			logRGB();
    			eepromLog();
    			measCount = 0;
    			RGBreset();
    		}
    	}
    	break;

    //default to log mode if status gets screwed up
    default:
    	eepromGetPeriod();
    	measCount = 0;
    	setupADXL();
    	ADXLreset();
    	RGBreset();
    	eepromFindAddress();
    	eepromSetStatus(0x52);
    }

}


