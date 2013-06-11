//This file implements the basic i2c functions

#include "msp430.h"
#include "i2c.h"

unsigned char i2caddress;
unsigned char tdata[258];
unsigned char rdata[66];

//setup i2c
void i2cActivate() {
	_DINT();
	P3SEL |= 0x06;                            // Assign I2C pins to USCI_B0
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 50;                             // fSCL = SMCLK/50 = ~80kHz for 4MHz DCO
	UCB0BR1 = 0;
	UCB0I2CSA = i2caddress;                      // set slave address
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	_BIS_SR(GIE);           		 		  // General Interrupt Enable
}

//transmit data over i2c
void i2cTransmit(unsigned char *data, int datalength, int stop) {
	UCB0I2CSA = i2caddress;
	UCB0IE |= UCTXIE;
	UCB0IE &= ~UCRXIE;
	UCB0CTL1 |= UCTR + UCTXSTT;                   // I2C TX, start condition
	int i = 0;
	for(i = 0; i < datalength; i++) {
		UCB0TXBUF = data[i];   	  				  // Load TX buffer

		//turn off CPU while waiting for next
		UCB0IE |= UCTXIE;                          // Enable TX interrupt
		LPM0;									  //enter low power mode
	}

	if(stop) {
		UCB0CTL1 |= UCTXSTP;
		while (UCB0CTL1 & UCTXSTP);		  //ensure stop condition was sent
	}
	UCB0IE &= ~UCTXIE;
}

//receive data over i2c
void i2cReceive(unsigned char *data, int datalength) {
	UCB0I2CSA = i2caddress;
	UCB0CTL1 &= ~UCTR;
	UCB0IE &= ~UCTXIE;
	UCB0IE |= UCRXIE;
	UCB0CTL1 |= UCTXSTT;		  // I2C TX, start condition
	data[0] = UCB0RXBUF;

	//turn off CPU while waiting for next
	UCB0IE |= UCRXIE;                          // Enable RX interrupt
	LPM0;
	int i = 0;
	for(i = 0; i < datalength; i++) {
		data[i] =  UCB0RXBUF;

		//turn off CPU while waiting for next
		UCB0IE |= UCRXIE;                          // Enable RX interrupt
		LPM0;									  //enter low power mode
	}
	UCB0CTL1 |= UCTXSTP;
	while (UCB0CTL1 & UCTXSTP);		  //ensure stop condition was sent
	UCB0IE &= ~UCRXIE;
}

//i2c Tx/Rx interrupt vector
//on completion of Tx/Rx should clear interrupt flags and exit LPM
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
  switch(__even_in_range(UCB0IV,12))
  {
  case  0: break;                           // Vector  0: No interrupts
  case  2: break;                           // Vector  2: ALIFG
  case  4: break;                           // Vector  4: NACKIFG
  case  6: break;                           // Vector  6: STTIFG
  case  8: break;                           // Vector  8: STPIFG
  case 10: 	                           // Vector 10: RXIFG
	  UCB0IFG &= ~UCRXIFG;                  // Clear USCI_B0 TX int flag
	  __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
	  break;
  case 12:                                  // Vector 12: TXIFG
	  UCB0IFG &= ~UCTXIFG;                  // Clear USCI_B0 TX int flag
	  __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
    break;
  default: break;
  }
}



