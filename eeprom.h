

/*
 * eeprom.h
 *
 *  Created on: May 2, 2012
 *      Author: hamner
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#define MAX_RECORD_ADDRESS 131064 // was 32760 for testing	// EEPROM maximum address, EEPROM size in bytes - 8 bytes for last record
extern char eepromStatus;
extern long int eepromAddress;
extern int eepromPeriod;

void eepromSetStatus(unsigned char Status);
unsigned char eepromGetStatus();
void eepromGetPeriod();
void logBatteryHours();
void eepromWrite(unsigned char data[], int length, unsigned long add);
void eepromRead(unsigned char data[], int length, unsigned long add);
void eepromReadLBA(unsigned long add, unsigned char* data);
void eepromWriteLBA(unsigned long add, unsigned char* data);
void flashWriteLBA(unsigned char* flashAddr, unsigned char* data);
unsigned char WriteLBA(unsigned long LBA, unsigned char *buff, unsigned char lbaCount);
unsigned char ReadLBA(unsigned long LBA, unsigned char *buff,  unsigned char lbaCount);
void eepromLog();
void eepromReset();
void eepromFindAddress();
void eepromMarkReset();
int eepromAscii2Num(int lineNum);
void BatteryVoltage(void);


#endif /* EEPROM_H_ */
