//This handles the watchdog, timer A (add later), and timer B

#include "timers.h"
#include "msp430.h"

char timerAevent;
char USBtiming;

//set watchdog to 250 ms as per p. 386 in the programmer's guide
void wdReset_250() {
	WDTCTL = WDTPW + 0x2D;
}

//set watchdog to 1 second as per p. 386 in the programmer's guide
void wdReset_1000() {
	WDTCTL = WDTPW + 0x2C;
}

//set watchdog to 16 seconds as per p. 386 in the programmer's guide
void wdReset_16000() {
	WDTCTL = WDTPW + 0x2B;
}

//turns off watchdog timer
void wdOff() {
    WDTCTL = WDTPW + WDTHOLD;
}

//timerB is used to delay a fixed amount of time in LPM3
void tbDelay(float milliseconds) {
	TBCTL = TBSSEL_1 + TBCLR;                 // ACLK, clear TBR
	_BIS_SR(GIE);           // General Interrupt Enable
	TBCCTL0 = CCIE;                           // TBCCR0 interrupt enabled
	TBCCR0 = (8*(milliseconds/1000)*4096) - 1;
	TBCTL = TBSSEL_1 + MC_1;
	if(USBtiming) {
		LPM0; 									// After debugging switch back to LPM3
	}
	else {
		LPM3;
	}
	TBCCTL0 &= ~CCIE;
}

//timerB is used to delay a fixed amount of time in LPM3
void tbDelayMode(float milliseconds, int sleepMode) {
	TBCTL = TBSSEL_1 + TBCLR;                 // ACLK, clear TBR
	_BIS_SR(GIE);           // General Interrupt Enable
	TBCCTL0 = CCIE;                           // TBCCR0 interrupt enabled
	TBCCR0 = (8*(milliseconds/1000)*4096) - 1;
	TBCTL = TBSSEL_1 + MC_1;
	if(sleepMode == 3) {
		LPM3;
	}
	else {
		LPM0;
	}
	TBCCTL0 &= ~CCIE;
}

//setup and start timerA
void taStart() {
	TA0CTL = TASSEL_1 + TACLR;   // ACLK, clear TAR
	TA0CCTL0 = CCIE;             // CCR0 capture and compare interrupt enabled
    TA0CCR0  = 32768-1;           //interrupt rate at 1Hz rate (ACLK @ 32768 Hz)
	TA0CTL = TASSEL_1 + MC_1;    // ACLK source, upmode, start timer
}

//stop timerA
void taStop() {
	TA0CCTL0 &= ~CCIE;           // CCR0 capture and compare interrupt disabled
	TA0CTL = TASSEL_1 + MC_0;    // ACLK source, stop timer
}

// Timer B0 interrupt service routine
#pragma vector=TIMERB0_VECTOR
__interrupt void TIMERB0_ISR (void){
	if(USBtiming) {
		LPM0_EXIT; 								// After debugging switch back to LPM3
	}
	else {
		LPM3_EXIT;
	}
}

// Timer0 A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
	timerAevent = 1;
	LPM3_EXIT;
}



