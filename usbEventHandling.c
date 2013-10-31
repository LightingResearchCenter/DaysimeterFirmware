//(c)2009 by Texas Instruments Incorporated, All Rights Reserved.
/*----------------------------------------------------------------------------+
 |                                                                             |
 |                              Texas Instruments                              |
 |                                                                             |
 |                          MSP430 USB-Example (CDC/HID Driver)                |
 |                                                                             |
 +-----------------------------------------------------------------------------+
 |  Source: usbEventHandling.c, File Version 1.00 2009/12/03                  |
 |  Author: RSTO                                                               |
 |                                                                             |
 |  Description:                                                               |
 |  Event-handling placeholder functions.                                      |
 |  All functions are called in interrupt context.                              |
 |                                                                             |
 +----------------------------------------------------------------------------*/

#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"
#include "USB_API/USB_Common/defMSP430USB.h"
#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/usb.h"
#include "F5xx_F6xx_Core_Lib/HAL_UCS.h"
#include "eeprom.h"
#include "AndyMscFseData32kEEPROM.h"
#include "timers.h"
#include "i2c.h"
#include "ADXL.h"

#ifdef _CDC_
#include "USB_API/USB_CDC_API/UsbCdc.h"
#endif

#ifdef _HID_
#include "USB_API/USB_HID_API/UsbHid.h"
#endif

#ifdef _MSC_
#include "USB_API/USB_MSC_API/UsbMsc.h"
#endif

#ifdef _PHDC_
#include "USB_API/USB_PHDC_API/UsbPHDC.h"
#endif

extern unsigned char logFlag;

//These variables are only example, they are not needed for stack
extern volatile BYTE bHIDDataReceived_event;    //data received event

/*
 * If this function gets executed, it's a sign that the output of the USB PLL has failed.
 * returns TRUE to keep CPU awake
 */
BYTE USB_handleClockEvent ()
{
    //TO DO: You can place your code here

    return (TRUE);                              //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
 * If this function gets executed, it indicates that a valid voltage has just been applied to the VBUS pin.
 * returns TRUE to keep CPU awake
 */
BYTE USB_handleVbusOnEvent ()
{
    //TO DO: You can place your code here
	// Stop timerA
	// wdOff();
	wdReset_1000();
	//taStop();
	USBtiming = 1;
	//BatteryVoltage();  // read battery voltage and store to line 7 of log_info.txt

    //We switch on USB and connect to the BUS
	if ((USB_enable() == kUSB_succeed) && (logFlag == 0)){
    // if (USB_enable() == kUSB_succeed){
		taStop();
		BatteryVoltage();  // read battery voltage and store to line 7 of log_info.txt
        USB_reset();
        int i = 0;
        for(;i<10000;i++); // try a delay here
        USB_connect();                          //generate rising edge on DP -> the host enumerates our device as full speed device
    }
    else if (logFlag == 1) logFlag = 0;
    return (TRUE);                              //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
 * If this function gets executed, it indicates that a valid voltage has just been removed from the VBUS pin.
 * returns TRUE to keep CPU awake
 */
BYTE USB_handleVbusOffEvent ()
{
    //TO DO: You can place your code here

    XT2_Stop();
    USB_disable();

    // Copy Constant Root Directory and FAT to active Root Directory and FAT
    flashWriteLBA((BYTE*)Root_Dir,(BYTE*)Root_Dir_Const);
    flashWriteLBA((BYTE*)FAT,(BYTE*)FAT_Const);
    // Enable/re-enable timerA
    taStart();
    i2cActivate();

    //wdReset_1000();
    eepromGetStatus();
    eepromGetPeriod();
    if(eepromStatus == 48) {
    	stopADXL();
    }

    return (TRUE);                              //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
 * If this function gets executed, it indicates that the USB host has issued a USB reset event to the device.
 * returns TRUE to keep CPU awake
 */
BYTE USB_handleResetEvent ()
{
    //TO DO: You can place your code here

	return (TRUE);                              //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
	}

/*
 * If this function gets executed, it indicates that the USB host has chosen to suspend this device after a period of active
 * operation.
 * returns TRUE to keep CPU awake
 */
BYTE USB_handleSuspendEvent ()
{
    //TO DO: You can place your code here

    return (TRUE);                              //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
 * If this function gets executed, it indicates that the USB host has chosen to resume this device after a period of suspended
 * operation.
 * returns TRUE to keep CPU awake
 */
BYTE USB_handleResumeEvent ()
{
    //TO DO: You can place your code here

    return (TRUE);                              //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

/*
 * If this function gets executed, it indicates that the USB host has enumerated this device :
 * after host assigned the address to the device.
 * returns TRUE to keep CPU awake
 */
BYTE USB_handleEnumCompleteEvent ()
{
    //TO DO: You can place your code here

    return (TRUE);                              //return TRUE to wake the main loop (in the case the CPU slept before interrupt)
}

#ifdef _CDC_

/*
 * This event indicates that data has been received for interface intfNum, but no data receive operation is underway.
 * returns TRUE to keep CPU awake
 */
BYTE USBCDC_handleDataReceived (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that a send operation on interface intfNum has just been completed.
 * returns TRUE to keep CPU awake
 */
BYTE USBCDC_handleSendCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that a receive operation on interface intfNum has just been completed.
 */
BYTE USBCDC_handleReceiveCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that new line coding params have been received from the host
 */
BYTE USBCDC_handleSetLineCoding (BYTE intfNum, ULONG lBaudrate)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that new line state has been received from the host
 */
BYTE USBCDC_handleSetControlLineState (BYTE intfNum, BYTE lineState)
{
	return FALSE;
}

#endif //_CDC_

#ifdef _HID_
/*
 * This event indicates that data has been received for interface intfNum, but no data receive operation is underway.
 * returns TRUE to keep CPU awake
 */
BYTE USBHID_handleDataReceived (BYTE intfNum)
{
    //TO DO: You can place your code here

    bHIDDataReceived_event = TRUE;              //data received event

    return (TRUE);                              //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that a send operation on interface intfNum has just been completed.
 * returns TRUE to keep CPU awake
 */
BYTE USBHID_handleSendCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that a receive operation on interface intfNum has just been completed.
 */
BYTE USBHID_handleReceiveCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that a request to switch to or from Boot protocol
 * was received from the host
 */
BYTE USBHID_handleBootProtocol (BYTE protocol, BYTE intfnum)
{
    return (FALSE);
}

#endif //_HID_

#ifdef _MSC_
BYTE USBMSC_handleBufferEvent (VOID)
{
    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

#endif //_MSC_


#ifdef _PHDC_

/*
 * This event indicates that data has been received for interface intfNum, but no data receive operation is underway.
 * returns TRUE to keep CPU awake
 */
BYTE USBPHDC_handleDataReceived (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (TRUE);                              //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that a send operation on interface intfNum has just been completed.
 * returns TRUE to keep CPU awake
 */
BYTE USBPHDC_handleSendCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

/*
 * This event indicates that a receive operation on interface intfNum has just been completed.
 */
BYTE USBPHDC_handleReceiveCompleted (BYTE intfNum)
{
    //TO DO: You can place your code here

    return (FALSE);                             //return FALSE to go asleep after interrupt (in the case the CPU slept before
                                                //interrupt)
}

#endif //_PHDC_

/*----------------------------------------------------------------------------+
 | End of source file                                                          |
 +----------------------------------------------------------------------------*/
/*------------------------ Nothing Below This Line --------------------------*/
