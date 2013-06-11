// Firmware code for Daysimeter12 (USB Daysimeter)
// Andrew Bierman and Robert Hamner
// August 2012
// Revised 21-Sep-2012
// Revised 03-Apr-2013  rangeFlag added to prevent 16-bit rollover, high range values divided by 10

#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"          
#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/usb.h"       
#include "F5xx_F6xx_Core_Lib/HAL_UCS.h"
#include "F5xx_F6xx_Core_Lib/HAL_PMM.h"
#include "F5xx_F6xx_Core_Lib/HAL_FLASH.h"
#include <string.h>

// MSC #includes
#include "USB_API/USB_MSC_API/UsbMscScsi.h"
#include "USB_API/USB_MSC_API/UsbMsc.h"
#include "USB_API/USB_MSC_API/UsbMscStateMachine.h"
#include "AndyMscFseData32kEEPROM.h"
#include "log.h"
#include "timers.h"
#include "eeprom.h"
#include "i2c.h"
#include "ADXL.h"
#include "opticalsensor.h"

unsigned char logFlag = 0;
struct config_struct USBMSC_config = {
    0x00,     // The number of this LUN.  (Ignored in this version of the API.)
    0x00,     // PDT (Peripheral Device Type). 0x00 = "direct-access block device", which includes all magnetic media.  (Ignored in this version of the API.)
    0x80,     // Indicates whether the medium is removable.  0x00 = not removable, 0x80 = removable
    "TI",     // T10 Vendor ID.  Generally not enforced, and has no particular impact on most operating systems.  
    "MSC",    // T10 Product ID.  Selected by the owner of the vendor ID above.  Has no impact on most OSes.  
    "v3.0"   // T10 revision string.  Selected by the owner of the vendor ID above.  Has no impact on most OSes.
};

void Init_StartUp();               // Initialize clocks, power, and I/Os

// The number of bytes per block.  In FAT, this is 512
const WORD BYTES_PER_BLOCK = 512;

// Data-exchange buffer between the API and the application.  The application allocates it, 
// and then registers it with the API.  Later, the API will pass it back to the application when it
// needs the application to "process" it (exchange data with the media).  
//__no_init BYTE RW_dataBuf[512 * 2];
BYTE RW_dataBuf[512 * 2];
BYTE *PRW_dataBuf; 					// Pointer to RW_dataBuf

// The API allocates an instance of structure type USBMSC_RWbuf_Info to hold all information describing  
// buffers to be processed.  The structure instance is a shared resource between the API and application.  
// During initialization, we'll call USBMSC_fetchInfoStruct() to obtain the pointer from the API.    
USBMSC_RWbuf_Info *RWbuf;

volatile BYTE timerAEvent = 0;  // Flag indicating TimerA meet timing criterion
unsigned char testa;
unsigned char testb[32];
long int i = 0;

/*----------------------------------------------------------------------------+
| Main Routine                                                                |
+----------------------------------------------------------------------------*/
VOID main(VOID)
{
	wdOff();

    Init_StartUp();     	// Initialize clocks, power, I/Os
    wdReset_16000(); 		// activate watchdog, 16-second interval
    //USBtiming = 1; 	// Use LPM0 (not LPM3)
    tbDelayMode(1000,0); 	// 1000 ms delay, LPM0
    //for(i=0;i<20;i++){
    //    	P1OUT |= BIT0;          // P1.0 = high, turn on LED
    //    	tbDelay(200);
    //    	P1OUT &= ~BIT0;         // P1.0 = low, turn off LED
    //    	tbDelay(200);
    //    }
/*
    //force a reset on startup
    eepromRead(rdata, 1, 0x0A);
    if(rdata[0] < 1) {
    	tdata[0] = rdata[0] + 1;
    	eepromWrite(tdata, 1, 0x0A);
    	//wdReset_250();
    	//tbDelay(300);
    	//PMMCTL0 |= PMMPW; 		// Password for register access
    	//PMMCTL0 |= PMMSWBOR; 	// cause a BOR

    	//P1OUT &= ~0x08; 	//P1.3 low to cause reset
    }
    else {
    	tdata[0] = 0;
    	eepromWrite(tdata, 1, 0x0A);
    }
*/
    //restart logging if needed
    wdReset_1000();
    eepromGetPeriod();
    //eepromPeriod /= 15; 	// REMOVE THIS AFTER TESTING!!!
    if(eepromGetStatus() == 0x34) {
    	eepromFindAddress();
    	eepromMarkReset();
    	setupADXL();
    	ADXLreset();
    	RGBreset();
    }
    //wdOff();
    wdReset_1000();

    __enable_interrupt(); 
    
    USB_init();                     // Initialize the USB module

    // Enable all USB events
    USB_setEnabledEvents(kUSB_allUsbEvents);

    
    // The data interchange buffer (used when handling SCSI READ/WRITE) is declared by the application, and 
    // registered with the API using this function.  This allows it to be assigned dynamically, giving 
    // the application more control over memory management.  
    USBMSC_registerBufInfo(0, &RW_dataBuf[0], NULL, sizeof(RW_dataBuf));
   
    
    // The API maintains an instance of the USBMSC_RWbuf_Info structure.  If double-buffering were used, it would
    // maintain one for both the X and Y side.  (This version of the API only supports single-buffering,
    // so only one structure is maintained.)  This is a shared resource between the API and application; the 
    // application must request the pointers.  This function returns the pointer for a given LUN and buffer side.  
    RWbuf = USBMSC_fetchInfoStruct();  // 0 for X-buffer
    
    
    // The application must tell the API about the media.  Since the media isn't removable, this is only called 
    // once, at the beginning of execution.  If the media were removeable, the application must call this any time
    // the status of the media changes.  
    struct USBMSC_mediaInfoStr mediainfo; // This struct type contains information about the state of the medium.   
                                          // Since it's only used locally, it's declared within main so that it's 
                                          // taken from the heap rather than as a static global.
    mediainfo.mediaPresent = 0x01;        // The medium is present, because internal flash is non-removable.  
    mediainfo.mediaChanged = 0x00;        // It can't change, because it's in internal memory, which is always present.
    mediainfo.writeProtected = 0x00;      // It's not write-protected
    mediainfo.lastBlockLba = 261;         // 256 blocks in the 1 Mbit EEPROM + 5 blicks in flash (64+5 for 32kEEPROM) (This number is also found twice in the volume itself; see mscFseData.c. They should match.)
    mediainfo.bytesPerBlock = BYTES_PER_BLOCK; // 512 bytes per block. (This number is also found in the volume itself; see mscFseData.c. They should match.)
    USBMSC_updateMediaInfo(0, &mediainfo); 
    
    // If USB is already connected when the program starts up, then there won't be a USB_handleVbusOnEvent(). 
    // So we need to check for it, and manually connect if the host is already present.
    wdReset_1000();
    if (USB_connectionInfo() & kUSB_vbusPresent)
    {
        if (USB_enable() == kUSB_succeed)
        {
            USB_reset();
            USB_connect();
        }
    }
    else{
    	USB_disable();
    	XT2_Stop();
    }
    /*  // This was important when using MSP430 internal flash to hold data. For setting initial value and erasing before writing log - 30Apr2012
    reStartPtr = (unsigned char *)0x1880; 		// Start of flash information memory C
    if (*reStartPtr==0xFF){
    	MSData[0] = 0x00;
    	writeDataToFlash(reStartPtr,MSData,1);
    	WDTCTL = WDTPW + 0x07;	    // Start watchdog timer,WDTIS = 0x07 = /64 (SMCLK)
    	for (i=0;i<30000;i++); 		// WDT should cause program to reset now
    }
    else{
    	eraseFlashLogData(reStartPtr,1);
    }

    //StatusPtr = (unsigned char *)STATUS_ADDR;
	//resumeFlag = *StatusPtr; 			// Check to see if device was previously logging
	// if (*StatusPtr==0x04 || *StatusPtr==0x34) findRecordAddress();
    //resumeFlag = 0x00;
   */

    //RWbuf->bufferAddr = (BYTE*)0x2420;
    //ReadLBA(1, RWbuf->bufferAddr,  1);

    while(1)
    {
        switch(USB_connectionState())
        {
        case ST_USB_DISCONNECTED:
             //__bis_SR_register(LPM3_bits + GIE); 	       // Enter LPM3 until VBUS-on event, or TimerA interrupt
        	wdReset_16000();
        	_BIS_SR(GIE);
        	LPM3;
             i++;

             // Check if the reason we woke was TimerA; and if so, log a new piece of data. 
             __disable_interrupt();
             //if(timerAevent && !(USB_connectionInfo() & kUSB_vbusPresent)) {
             if (timerAevent) {
             	logFlag = 1; 			// Flag usbEventHandling not to connect until logData finishes
             	__enable_interrupt();
             	//P1OUT |= BIT0;          // P1.0 = high, turn on LED
          		wdReset_1000();
             	//USBtiming = 0;
             	log();
             	//USBtiming = 1;
          		wdReset_1000();
             	//P1OUT &= ~BIT0;         // P1.0 = low, turn off LED
                timerAevent = 0;
             	//if ((USB_enable() == kUSB_succeed) && (logFlag == 0)){ 	// if usbVbusOnEvent ocurred during log. Calling this function starts the USB PLL drawing lots of current
                if (logFlag == 0) { 	// if usbVbusOnEvent ocurred during log
                	//wdOff();
                	taStop();
                	if (USB_enable() == kUSB_succeed){
                		//int i = 0;
                		//for(;i<10000;i++); // try a delay here
                		USB_reset();
                		USB_connect();  // generate rising edge on DP -> the host enumerates our device as full speed device
                	}
    			}
             	logFlag = 0;
             	
             }
             else __enable_interrupt();
             break;
            
        case ST_USB_CONNECTED_NO_ENUM:
        	 wdReset_1000();
             break;
            
        case ST_ENUM_ACTIVE:

    			//if(P1OUT & 0x01) {
    			//	P1OUT |= 0x00;
    			//}
    			//else {
    			//	P1OUT |= 0x01;
    			//}

        		wdReset_1000();
                // Call USBMSC_poll() to initiate handling of any received SCSI commands.  Disable interrupts 
                // during this function, to avoid conflicts arising from SCSI commands being received from the host
                // AFTER decision to enter LPM is made, but BEFORE it's actually entered (avoid sleeping accidentally).  
                __disable_interrupt();
                if(USBMSC_poll() == kUSBMSC_okToSleep)
                {
                	wdOff();
                    __bis_SR_register(LPM0_bits + GIE);  // Enable interrupts atomatically with LPM0 entry
                }
                wdReset_1000();
                __enable_interrupt();
                                   
                // If the API needs the application to process a buffer, it will keep the CPU awake by returning kUSBMSC_processBuffer
                // from USBMSC_poll().  The application should respond by checking the 'operation' field of all defined USBMSC_RWbuf_Info
                // structure instances.  If any of them is non-null, then an operation needs to be processed.  A value of 
                // kUSBMSC_READ indicates the API is waiting for the application to fetch data from the storage volume, in response 
                // to a SCSI READ command from the USB host.  After doing so, we must indicate whether the operation succeeded, and 
                // close the buffer operation by calling USBMSC_bufferProcessed().  
                while(RWbuf->operation == kUSBMSC_READ)
                {
                	wdReset_1000();
              	 // P1OUT &= ~BIT4;         // P1.0 = low, turn off LED
              	 // tbDelay(250);
              	 // P1OUT |= BIT4;          // P1.0 = high, turn on LED

                  RWbuf->returnCode = ReadLBA(RWbuf->lba, RWbuf->bufferAddr, RWbuf->lbCount); // Fetch a block from the medium, using file system emulation
                  USBMSC_bufferProcessed();                           // Close the buffer operation
                }
                
                // Same as above, except for WRITE.  If operation == kUSBMSC_WRITE, then the API is waiting for us to 
                // process the buffer by writing the contents to the storage volume.  
                while(RWbuf->operation == kUSBMSC_WRITE)
                {
                  wdReset_1000();
                  RWbuf->returnCode = WriteLBA(RWbuf->lba, RWbuf->bufferAddr, RWbuf->lbCount); // Write the block to the medium, using file system emulation
                  USBMSC_bufferProcessed();                            // Close the buffer operation
                }
                break;
                
            case ST_ENUM_SUSPENDED:
            	wdOff();
            	//wdReset_1000();
                 __bis_SR_register(LPM3_bits + GIE);            // Enter LPM3, until a resume or VBUS-off event
                 break;
                
            case ST_ENUM_IN_PROGRESS:
            	 wdReset_1000();
                 break;
                
            case ST_ERROR:
                 break;
                
            default:;
        }    
    }  // while(1)
} //main()


/*----------------------------------------------------------------------------+
| System Initialization Routines                                              |
+----------------------------------------------------------------------------*/

// This function initializes the F5xx Universal Clock System (UCS):
// MCLK/SMCLK:  driven by the DCO/FLL, set to USB_MCLK_FREQ (a configuration constant defined by the Descriptor Tool) 8 Mhz Default, 4 Mhz crystal
// ACLK:  the internal REFO oscillator
// FLL reference:  the REFO
// This function anticipates any MSP430 USB device (F552x/1x/0x, F563x/663x), based on the project settings.  

VOID Init_Clock(VOID)
{
#   if defined (__MSP430F563x_F663x)
		while(BAKCTL & LOCKIO)                    // Unlock XT1 pins for operation
      	BAKCTL &= ~(LOCKIO);                    // enable XT1 pins
     	// Workaround for USB7
    	UCSCTL6 &= ~XT1OFF;
#   endif
    
    //Initialization of clock module
    if (USB_PLL_XT == 2)
    {
#       if defined (__MSP430F552x) || defined (__MSP430F550x)
        	P5SEL |= 0x0C;                        // enable XT2 pins for F5529
#       elif defined (__MSP430F563x_F663x)
			P7SEL |= 0x0C;
#       endif
        // use REFO for FLL and ACLK
        //UCSCTL3 = (UCSCTL3 & ~(SELREF_7)) | (SELREF__REFOCLK);
        UCSCTL3 = (UCSCTL3 & ~(SELREF_7)) | (SELREF__XT1CLK);
        //UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK);
        UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__XT1CLK); 	// SMCLK and MCLK keep default settings (DCO/2)
        //UCSCTL4 = 0x00; //set clocks to XT1 as per programmer's guide
         
        Init_FLL(USB_MCLK_FREQ/1000, USB_MCLK_FREQ/32768);             // set FLL (DCOCLK)
        XT2_Start(XT2DRIVE_3);
    }
    else 	// This case is not used because we are using a separate crystal for USB_PLL
    {
#       if defined (__MSP430F552x) || defined (__MSP430F550x)
            P5SEL |= 0x10;                    // enable XT1 pins
#       endif 
        UCSCTL3 = SELREF__REFOCLK;            // run FLL mit REF_O clock
        //UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK); // set ACLK = REFO
        //UCSCTL4 = 0xA4;
        //UCSCTL4 = 0x00; //set clocks to XT1 as per programmer's guide
        Init_FLL(USB_MCLK_FREQ/1000, USB_MCLK_FREQ/32768);       // set FLL (DCOCLK)
        XT1_Start(XT1DRIVE_3);
    }
}

//----------------------------------------------------------------------------

// This function initializes the I/Os.  It expects to be running on an MSP430 FET target board; so
// if using on another board, it's a good idea to double-check these settings.  It sets most of the
// I/Os as outputs, to eliminate floating inputs, for optimum power usage.  
// This function anticipates any MSP430 USB device (F552x/1x/0x, F563x/663x), based on the project settings.  

VOID Init_Ports(VOID)
{
    // Initialization of ports all unused pins as outputs with low-level

		P1OUT = 0x17; 	// P1.2 high for simulated ADC input, P1.4 high for power p3 (reference)
	    P1SEL = 0x00;
	    P1DIR = 0xFF; 	// All outputs

	    P2OUT = 0xC0; 	// P2.6 and P2.7 high for power ps1 and LEDs, all others low
	    P2SEL = 0x00;
	    P2DIR = 0xFF; 	// All outputs

	    P3OUT    =  0x00;
	    P3SEL   |=  0x03;      // Assign I2C pins to USCI_B0
	    P3DIR    =  0xFC;      // All other pins  to output

	    P4OUT   =   0x01; 	// P4.0 high for power ps2 (I2C pullup)
	    P4SEL   =   0x00;
	    P4DIR   =   0xFF; 	// All outputs

	    P5OUT = 0x00;
	    P5SEL = 0x3F; 	// P5.0, P5.1 external reference input, P5.2, P5.3 XT2, P5.4, P5.5 XT1
	    //P5SEL = 0x3C; 	// P5.2, P5.3 XT2, P5.4, P5.5 XT1, when not using external reference
	    P5DIR = 0xFF; 	// All outputs

	    P6OUT = 0x00;
	    P6SEL = 0x07; 	// P6.0, P6.1, P6.2, P6.3, P6.4 selected for analog inputs A0, A1, A2, A3, A4
	    P6DIR = 0xF8; 	// P6.0, P6.1, P6.2, P6.3, P6.4 Inputs, All others Outputs

	    PJDIR   =   0xFFFF;
	    PJOUT   =   0x0000;
}


//----------------------------------------------------------------------------

VOID Init_StartUp(VOID)
{
   //unsigned short bGIE;
   //bGIE  = (__get_SR_register() &GIE);  //save interrupt status

   //__disable_interrupt();               // Disable global interrupts
    
    Init_Ports();                        // Init ports (do first ports because clocks do change ports)
    SetVCore(3);                         // USB core requires the VCore set to 1.8 volt, independ of CPU clock frequency
    Init_Clock();

    taStart();
    i2cActivate();
    
    //__bis_SR_register(bGIE); //restore interrupt status
}


#pragma vector = UNMI_VECTOR
__interrupt VOID UNMI_ISR(VOID)
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG))
    {
    case SYSUNIV_NONE:
      __no_operation();
      break;
    case SYSUNIV_NMIIFG:
      __no_operation();
      break;
    case SYSUNIV_OFIFG:
      UCSCTL7 &= ~(DCOFFG+0+0+XT2OFFG); // Clear OSC flaut Flags fault flags
      SFRIFG1 &= ~OFIFG;                                // Clear OFIFG fault flag
      break;
    case SYSUNIV_ACCVIFG:
      __no_operation();
      break;
    case SYSUNIV_BUSIFG:

      // If bus error occurred - the cleaning of flag and re-initializing of USB is required.
      SYSBERRIV = 0;            // clear bus error flag
      USB_disable();            // Disable
    }
}

/*----------------------------------------------------------------------------+
| End of source file                                                          |
+----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/

