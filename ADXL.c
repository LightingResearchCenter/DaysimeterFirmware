#include "i2c.h"
#include "timers.h"

unsigned int numReadings;
int xAccel[32],yAccel[32],zAccel[32];
unsigned char RxBuffer[32];
unsigned char MSData[10];
long int sumX, sumY, sumZ, sumXsq, sumYsq, sumZsq, meanX, meanY, meanZ;
long int totalnumReadings;
long int activityArray[6];
long int activityLong;
int Activity;
int meas30Count = 0;

void setupADXL(void) {
	i2caddress = 0x53;
	tdata[0] = 0x2C;						// Data rate and power mode control register
	tdata[1] = 0x16;						// Low power mode, 6.25 Hz
	i2cTransmit(tdata, 2, 1);
	tbDelay(10);
	tdata[0] = 0x31;						// Data format control register
	tdata[1] = 0x09;						// Full resolution, range = +-16g
	i2cTransmit(tdata, 2, 1);
	tbDelay(10);
	tdata[0] = 0x38;						// FIFO buffer control register
	tdata[1] = 0x9F;						// Stream mode, samples = 31
	i2cTransmit(tdata, 2, 1);
	tbDelay(10);
	tdata[0] = 0x2D;						// Power control register
	tdata[1] = 0x08;						// 8 = measure, 0 = standby
	i2cTransmit(tdata, 2, 1);
	tbDelay(10);
}

// Stop ADXL345
void stopADXL() {
	i2caddress = 0x53;
	tdata[0] = 0x2D;						// Power control register
	tdata[1] = 0x00;						// 8 = measure, 0 = standby
	i2cTransmit(tdata, 2, 1);
}

//this reads the current values from the ADXL, and logs them
void readADXL() {
	int i;
	i2caddress = 0x53;
	tdata[0] = 0x38;                       			// FIFO status register (set to previous and read 2 bytes)
	i2cTransmit(tdata, 1, 1);

	//Receive process
	i2cReceive(rdata, 2);
	numReadings = (unsigned int)(0x3F & (rdata[1]));	// Number of readings available in FIFO is last 5 bits in register
	for (i=0;i<numReadings;i++) {
		tdata[0] = 0x32;                       		// Starting register address of accelerometer data
		i2cTransmit(tdata, 1, 1);

		//Receive process
		i2cReceive(rdata, 6);
		xAccel[i] = (int)(rdata[1]<<8);
		xAccel[i] += (int)(rdata[0]);
		yAccel[i] = (int)(rdata[3]<<8);
		yAccel[i] += (int)(rdata[2]);
		zAccel[i] = (int)(rdata[5]<<8);
		zAccel[i] += (int)(rdata[4]);

		sumX += (long int)xAccel[i];
		sumY += (long int)yAccel[i];
		sumZ += (long int)zAccel[i];
		sumXsq += (long int)xAccel[i]*xAccel[i];
		sumYsq += (long int)yAccel[i]*yAccel[i];
		sumZsq += (long int)zAccel[i]*zAccel[i];
		meanX = (meanX*totalnumReadings + (long int)xAccel[i])/(totalnumReadings+1);
		meanY = (meanY*totalnumReadings + (long int)yAccel[i])/(totalnumReadings+1);
		meanZ = (meanZ*totalnumReadings + (long int)zAccel[i])/(totalnumReadings+1);
		totalnumReadings++;

	}
}

//this fills the activity array that is used later to find "Activity"
void findActivityArray() {
	activityLong = sumXsq+meanX*meanX*totalnumReadings-2*meanX*sumX;
	activityLong += sumYsq+meanY*meanY*totalnumReadings-2*meanY*sumY;
	activityLong += sumZsq+meanZ*meanZ*totalnumReadings-2*meanZ*sumZ;
  	activityLong /= totalnumReadings;
  	activityArray[meas30Count] = (activityLong);
	meas30Count++;
}

//this determines the "Activity" value that gets sent to the EEPROM
void findActivity() {
	int i = 0;
	for (;i<meas30Count;i++){
  		activityLong += activityArray[i];	// reusing this variable. Reset to zero in if-block above
  	}
	activityLong /= (long int)meas30Count;
  	Activity = (int)(activityLong>>4);
	meas30Count = 0;
}

//clears the variables associated with the accelerometer
void ADXLreset() {
	sumX = sumY = sumZ = sumXsq = sumYsq = sumZsq = 0;
	meanX = meanY = meanZ = 0;
	totalnumReadings = 0;
	activityLong = 0;
}


