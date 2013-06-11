//This handles the basic EEPROM functions
// Revised by A Bierman 03Apr2013

#include "i2c.h"
#include "eeprom.h"
#include "timers.h"
#include "msp430.h"
#include "USB_API/USB_Common/types.h"
#include "USB_config/descriptors.h"
#include "F5xx_F6xx_Core_Lib/HAL_FLASH.h"
#include "USB_API/USB_MSC_API/UsbMscScsi.h"
#include "AndyMscFseData32kEEPROM.h"
#include <string.h>
#include "opticalsensor.h"
#include "ADXL.h"

extern const WORD BYTES_PER_BLOCK;
extern BYTE RW_dataBuf[512 * 2];
extern BYTE *PRW_dataBuf; 					// Pointer to RW_dataBuf

char eepromStatus = 0;
long int eepromAddress = 0;
int eepromPeriod = 0;

//this function sets status to the value passed to it and writes it to address 0
void eepromSetStatus(unsigned char Status) {
	//i2caddress = 0x50;
	//tdata[0] = 0;
	//tdata[1] = 0;
	//tdata[2] = Status;
	//i2cTransmit(tdata, 3, 1);
	//tbDelay(10);
	tdata[0] = Status;
	eepromWrite(tdata, 1, 0);
	eepromStatus = Status;
}

//this function reads the status from address 0
unsigned char eepromGetStatus() {
	//i2caddress = 0x50;
	//tdata[0] = 0;
	//tdata[1] = 0;
	//i2cTransmit(tdata, 2, 0);
	//tbDelay(10);
	//i2cReceive(rdata, 1);
	eepromRead(rdata, 1, 0);
	eepromStatus = rdata[0];
	return rdata[0];
}

//this gets the logging period from address 0
/*
void eepromGetPeriod() {
	//i2caddress = 0x50;
	eepromRead(rdata, 1, 0x07);
	//tdata[0] = 0;
	//tdata[1] = 7;
	//i2cTransmit(tdata, 2, 0);
	//tbDelay(10);
	//i2cReceive(rdata, 1);
	eepromPeriod= rdata[0];
}
*/
// This gets the logging period from line 4 of log_info.txt
void eepromGetPeriod() {
	eepromPeriod = eepromAscii2Num(4); // line number 4 of log_info.txt
	eepromPeriod = (eepromPeriod/30)*30;  // force number to be a multiple of 30
	if(eepromPeriod<30) eepromPeriod = 30;  // make sure number is at least 30
}
/*
	int i,value;
	char *result = NULL;

	eepromRead(rdata, 32, 0x00); //Read entire start of file
	result = strchr((char *)rdata, '\n'); // 0x0A is hex value of linefeed
	for(i=0;i<2;i++){
		result = strchr(++result, '\n'); // 0x0A is hex value of linefeed
	}
	eepromPeriod = 0;
	while (result[1] != 0x0A){  // while not equal to carriage return character
		if (result[1] > 0x2F && result[1] < 0x3A){  // if character is ascii numeral
			value = result[1] - 0x30;  // convert ascii to number
			eepromPeriod = eepromPeriod*10 + value;  // multiply by 10 and add next digit
		}
		result++;
	}
}
*/
// This returns the value of the number on the specified line of the log_info.txt file
int eepromAscii2Num(int lineNum){
	int i,value,number;
	char *result = NULL;

	eepromRead(rdata, 64, 0x00); //Read entire start of file
	result = strchr((char *)rdata, '\n'); // 0x0A is hex value of linefeed
	for(i=0;i<(lineNum-2);i++){
		result = strchr(++result, '\n'); // 0x0A is hex value of linefeed
	}
	number = 0;
	while (result[1] != 0x0A){  // while not equal to carriage return character
		if (result[1] > 0x2F && result[1] < 0x3A){  // if character is ascii numeral
			value = result[1] - 0x30;  // convert ascii to number
			number = number*10 + value;  // multiply by 10 and add next digit
		}
		result++;
	}
	return (number);
}

// This converts a number to ASCII and writes it on the specified line of the log_info.txt file replacing what is there
void eepromNum2Ascii(int number, int lineNum){
	int k,i,j,n; // int flag;
	char digit[5], str[7];

	for (k=3;k>=0;k--){         // 4 digit number
		digit[k] = number % 10;
		number /= 10;
	}
	// flag=0;  // for suppressing leading zeros
	n = 0;
	for (k=0;k<4;k++){
		// flag |= digit[k]; // to send zeros after most significant non-zero digit but not before
		// if (flag | k==3){
		str[n] = (digit[k] | 0x30);  // convert to ASCII
		n++;
		// }
	}
	str[n++] = 0x0D; // carriage return
	str[n++] = 0x0A; // line feed

	eepromRead(rdata, 64, 0x00); //Read entire start of file

	k = 0;
	for(i=0;i<(lineNum-1);i++){ // Locate start of line
		while(rdata[k++]!= '\n' && k<57);
	}
	j = k;
	while(rdata[j++]!= '\n' && j<57);   // Locate start of next line
	for(i=0;i<k;i++){  // Before insertion
		tdata[i] = rdata[i];
	}
	for(i=0;i<n;i++){  // Insertion
		tdata[i+k] = str[i];
	}
	for(i=0;i<(64-j);i++){  // After insertion
		tdata[i+k+n] = rdata[i+j];
	}

	eepromWrite(tdata, 64, 0); // write 64 characters of tdata starting at eeprom address 0
}

//this writes a log
void eepromLog() {
	//keep track of number of hours used on this battery
	if((eepromAddress % (28800/eepromPeriod)) == 0) {
		logBatteryHours();
	}

	// Store RGB range information in LSB of Activity (added 03Apr2013)
	Activity = 2*(Activity/2); 	// round to even digit (make LSB = 0)
	if(rangeFlag==1) Activity += 1;

	// logBatteryHours(); // For debugging. Remove for normal operation
	tdata[0] = ((Red>>8) < 0xFF ? (Red>>8) : 0xFE);
	tdata[1] = Red;
	tdata[2] = ((Green>>8) < 0xFF ? (Green>>8) : 0xFE);
	tdata[3] = Green;
	tdata[4] = ((Blue>>8) < 0xFF ? (Blue>>8) : 0xFE);
	tdata[5] = Blue;
	tdata[6] = ((Activity>>8) < 0xFF ? (Activity>>8) : 0xFE);
	//tdata[7] = 2*(Activity/2); // round to even digit (for backward compatibility with range indicator on Dimesimeter)
	tdata[7] = Activity; 	// Revised 03Apr2013
	eepromWrite(tdata, 8, eepromAddress);
	//tbDelay(10);
	eepromAddress += 8;
}
/*
void logBatteryHours() {
	//i2caddress = 0x50;
	eepromRead(rdata, 2, 0x08);
	int hour = (rdata[0]<<8) + rdata[1] + 1;
	tdata[0] = hour>>8;
	tdata[1] = hour;
	eepromWrite(tdata, 2, 0x08);
}
*/
void logBatteryHours() {
	int hours;

	 hours = eepromAscii2Num(5) + 1;
	 eepromNum2Ascii(hours, 5);
}

void BatteryVoltage(void) {
	int reading;
	reading = readBatVolt();
	eepromNum2Ascii(reading, 7); // write batery voltage to line 7 of log_info.txt
}

//this sets unused bytes to 255 (0xFF) for reset detection and EOF purposes
void eepromReset() {
	//i2caddress = 0x50;
	int i = 0;

	//handle everything past byte 1024 the same way
	eepromAddress = 1024;
	while(eepromAddress < MAX_RECORD_ADDRESS) {
		wdReset_1000();
		//tdata[0] = (unsigned char)(eepromAddress>>8);
		//tdata[1] = (unsigned char)eepromAddress;
		for(i = 0; i < 256; i++) {
			tdata[i] = 0xFF;
		}
		eepromWrite(tdata, 256, eepromAddress);
		//i2cTransmit(tdata, 258, 1);
		//tbDelay(10);
		eepromAddress += 256;

	}
}

//find current address after a reset
void eepromFindAddress() {
	int found = 0;
	unsigned char value = 0, valuelow = 0;
	long int low = 1024, high = MAX_RECORD_ADDRESS;
	i2caddress = 0x50;
	//eepromAddress = 1024;
	while(!found) {
		wdReset_1000();
		eepromAddress = 2*((low + high)/4);
		eepromRead(rdata, 1, eepromAddress);
		value = rdata[0];
		eepromRead(rdata, 1, eepromAddress - 2);
		valuelow = rdata[0];
		if((value == 255) && (valuelow != 255)) {
			found = 1;
		}
		else if((value == 255) && (eepromAddress > 1024)) {
			high = 2*(eepromAddress/2);
		}
		else if(eepromAddress <= 1024) {
			eepromAddress = 1024;
			found = 1;
		}
		else {
			low = 2*(eepromAddress/2);
		}
		if(eepromAddress >= MAX_RECORD_ADDRESS - 8) {
			found = 1;
		}
	}

	//back up 8 to allow find_address to work properly
	eepromAddress = (((eepromAddress/8) + 1)*8) - 8;

	if(eepromAddress <= 1024) {
		eepromAddress = 1024;
	}
}

//this marks resets with a purple reading
void eepromMarkReset() {
	//i2caddress = 0x50;
	//tdata[0] = eepromAddress>>8;
	//tdata[1] = eepromAddress;
	tdata[0] = 0xFE;
	tdata[1] = 0xFE;
	tdata[2] = 0x00;
	tdata[3] = 0x00;
	tdata[4] = 0xFE;
	tdata[5] = 0xFE;
	tdata[6] = 0x00;
	tdata[7] = 0x00;
	//i2cTransmit(tdata, 10, 1);
	//tbDelay(10);
	eepromWrite(tdata, 8, eepromAddress);
	eepromAddress += 8;
}

//this function writes stuff to the EEPROM starting at address
void eepromWrite(unsigned char data[], int length, unsigned long add) {
	unsigned char dummy[258];
	unsigned long relAdd;

	if((add > 65535)) { // 65535 is max address for 16-bit I2C command so I2C address is changed for higher addresses
		i2caddress = 0x51;
		relAdd = add - 65536;
	}
	else{
		i2caddress = 0x50;
		relAdd = add;
	}
	dummy[0] = (unsigned char)(relAdd>>8);
	dummy[1] = (unsigned char)(relAdd);
	int i = 0;
	for(; i < length; i++) {
		dummy[i + 2] = data[i];
	}
	i2cTransmit(dummy, length + 2, 1);
	tbDelay(10);
}

//this function reads stuff from the EEPROM starting at address and stores it in data
void eepromRead(unsigned char data[], int length, unsigned long add) {
	unsigned long relAdd;
	if((add > 65535)) { // 65535 is max address for 16-bit I2C command so I2C address is changed for higher addresses
		i2caddress = 0x51;
		relAdd = add - 65536;
	}
	else{
		i2caddress = 0x50;
		relAdd = add;
	}
	tdata[0] = (unsigned char)(relAdd>>8);
	tdata[1] = (unsigned char)(relAdd);
	i2cTransmit(tdata, 2, 0);
	tbDelay(1);
	i2cReceive(data, length);
	tbDelay(1);
}

void eepromReadLBA(unsigned long add, unsigned char* data) {
	PRW_dataBuf = data; 			// Start of shared RW buffer that is used by USB API
	eepromRead(PRW_dataBuf, 512, (long int)add); 				// Read 512 bytes of data)
	tbDelayMode(1,0);
}

void eepromWriteLBA(unsigned long add, unsigned char* data) {
	int i;
	//long int address = (long int)add;
	PRW_dataBuf = data; 			// Start of shared RW buffer that is used by USB API
	//for (i=0;i<8;i++) {
	//	eepromWrite(PRW_dataBuf + 64*i, 64, address); 				// Write 2 address bytes + 64 bytes of data
	//	tbDelay(10);
	//	address = address + 64;
	//}
	for (i=0;i<2;i++) {
		eepromWrite(PRW_dataBuf + 256*i, 256, add); 				// Write 2 address bytes + 256 bytes of data
		tbDelayMode(10,0);
		add = add + 256;
	}
}

// This function implements the "file system emulation" approach.  It reads a block from the "storage medium", which
// in this case is internal flash and external EEPROM.  It evaluates the logical block address (LBA) and then for internal flash
// uses memcpy() to exchange "lbaCount" blocks, and for external EEPROM it uses eepromReadLBA() and eepromWriteLBA()
// to exchange "labCount" blocks.
// The switch() statement can be thought of as a map to the volume's block addresses.
unsigned char ReadLBA(unsigned long LBA, unsigned char *buff,  unsigned char lbaCount)
{
    unsigned char i,ret = kUSBMSC_RWSuccess;
    for (i=0; i<lbaCount; i++)
    {
        switch(LBA)
        {
          // The Master Boot Record (MBR).
          // This is always the first sector.
          case 0:
              memcpy(buff, MBR, BYTES_PER_BLOCK);
              break;

          // Partition block
          // The contents of the MBR had indicated that the partition block start at 0x01.
          case 1:    // 0x01
              memcpy(buff, Partition, BYTES_PER_BLOCK);
              break;

          // File Allocation Tables
          // The FAT file system requires two copies of the FAT table.  In our example, we actually store only one and report
          // it in response to the host requesting either.  Note that the partition segement's "sectors per FAT" field indicated
          // the FAT tables were to be 0xF2 = 242 segments long; we only store the first one.  Note also that the number of segments
          // allotted in this map for both the first FAT table (285-43=242) and the second FAT table (527-285=242) are both that length.
          case 2:    // First FAT   0x02
          case 3:   // Second FAT   0x03
              memcpy(buff, FAT, BYTES_PER_BLOCK);
              break;

          // Root directory
          // This area is actually 32 blocks long, but this example only stores the first.
          case 4:   // 0x04
              memcpy(buff, Root_Dir, BYTES_PER_BLOCK);
              break;

          // The data blocks.  We store blocks 5-68, which is 32 clusters.  (Note that the partition segment
          // had indicated two blocks per cluster.)

          default:
              if (LBA < 261){ 	// For LBAs within the EEPROM address range
            	  eepromReadLBA(((LBA-5)*0x200),buff); 	// LBA 5 is at EEPROM address 0x00, LBA 6 is at 0x200, etc.
              }
              else memset(buff, 0x00, BYTES_PER_BLOCK); 	// For any other LBA, return a blank block
              break;
        }
        LBA++;
        buff += BYTES_PER_BLOCK;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------------
// This function implements the "file system emulation" approach.  It writes a block to the "medium".  It supports
// writes to the first block of the FAT table (which is sufficient for the number of limited numbers of files we
// support); the first block of the root directory (same); and blocks 559-565.  This is three data clusters,
// generally sufficient for three small files.
unsigned char WriteLBA(unsigned long LBA, unsigned char *buff, unsigned char lbaCount)
{
    unsigned char i,ret = kUSBMSC_RWSuccess;
    for (i=0; i<lbaCount; i++)
    {

        switch(LBA)
        {
          case 2:   // First FAT
          case 3:  // Second FAT
              flashWriteLBA((unsigned char*)FAT,buff);
              break;
          case 4:  // Root directory
              flashWriteLBA((unsigned char*)Root_Dir,buff);
              break;

          default:
              if (LBA < 261){ 	// For LBAs within the EEPROM address range
            	  eepromWriteLBA(((LBA-5)*0x200),buff); 	// LBA 5 is at EEPROM address 0x00, LBA 6 is at 0x200, etc.
              }
              // For any other LBA, do nothing (nothing gets written to flash or EEPROM)
              break;
        }
        LBA++;
        buff += BYTES_PER_BLOCK;
    }

    return ret;
}

void flashWriteLBA(unsigned char* flashAddr, unsigned char* data)
{
  WORD i;
  unsigned short bGIE;

  bGIE  = (__get_SR_register() & GIE);  //save interrupt status
  __disable_interrupt();

  // Erase the segment
  FCTL3 = FWKEY;                            // Clear the lock bit
  FCTL1 = FWKEY+ERASE;                      // Set the Erase bit
  *flashAddr = 0;                           // Dummy write, to erase the segment

  // Write the data to the segment
  FCTL1 = FWKEY+WRT;                        // Set WRT bit for write operation
  for (i = 0; i < 512; i++)
    *flashAddr++ = *data++;                 // Write the block to flash
  FCTL1 = FWKEY;                            // Clear WRT bit
  FCTL3 = FWKEY+LOCK;                       // Set LOCK bit

  __bis_SR_register(bGIE); //restore interrupt status
}


