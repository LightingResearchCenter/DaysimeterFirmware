/*
 * i2c.h
 *
 *  Created on: May 2, 2012
 *      Author: hamner
 */

#ifndef I2C_H_
#define I2C_H_

extern unsigned char i2caddress;
extern unsigned char tdata[258];
extern unsigned char rdata[66];

void i2cActivate();
void i2cTransmit(unsigned char *data, int datalength, int stop);
void i2cReceive(unsigned char *data, int datalength);

#endif /* I2C_H_ */
