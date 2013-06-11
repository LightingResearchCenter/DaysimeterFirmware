/*
 * opticalsensor.h
 *
 *  Created on: May 2, 2012
 *      Author: hamner
 *      Revised by A Bierman 03Apr2013
 */

#ifndef OPTICALSENSOR_H_
#define OPTICALSENSOR_H_

extern unsigned int Red, Green, Blue;
extern int rangeFlag; 	// added 03Apr2013
void readRGB(void);
void logRGB();
void RGBreset();
int readBatVolt(void);

#endif /* OPTICALSENSOR_H_ */
