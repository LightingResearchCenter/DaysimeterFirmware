// Revised by A bierman 03Apr2013

#include "opticalsensor.h"
#include "msp430.h"

#define   Num_of_ADCreadings   3
#define RANGE_MULT 40.2

volatile unsigned int A0results[Num_of_ADCreadings];
volatile unsigned int A1results[Num_of_ADCreadings];
volatile unsigned int A2results[Num_of_ADCreadings];
volatile unsigned int A11results[Num_of_ADCreadings];
unsigned int Red, Green, Blue;
unsigned int index;
long int redlong, greenlong, bluelong;
long int counts;
long int modecount = 0;
int rangeFlag = 0; 		// added 03Apr2013

//this reads the optical sensor using the ADC on the msp430
void readRGB(void) {
	int i;

	ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_12; 	// Turn on ADC12, set sampling time to 1024 ADC12OSC cycles (was ADC12SHT0_08)
	ADC12CTL1 = ADC12SHP+ADC12CONSEQ_3; 		// Use sampling timer, repeated sequence
	ADC12MCTL0 = (ADC12SREF_6 + ADC12INCH_0); 	// Vr+ = VeRef+ and Vr- = AVss
	ADC12MCTL1 = (ADC12SREF_6 + ADC12INCH_1); 	// Vr+ = VeRef+ and Vr- = AVss
	ADC12MCTL2 = (ADC12SREF_6 + ADC12INCH_2 + ADC12EOS); 	// Vr+ = VeRef+ and Vr- = AVss + end of conversion sequence

	index = 0; 								// Loop counter in ADC12 ISR
	ADC12IE = 0x04;                           // Enable ADC12IFG.2
	ADC12CTL0 |= ADC12ENC;                    // Enable conversions
	ADC12CTL0 |= ADC12SC;                     // Start convn - software trigger

	__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, Enable interrupts
	__no_operation();                         // For debugger
	Green = 0;
	Blue = 0;
	Red = 0;
	for(i=0;i<Num_of_ADCreadings;i++){
		Green += A0results[i];
		Blue += A2results[i];
		Red += A1results[i];
	}
	Green /= Num_of_ADCreadings;
	Blue /= Num_of_ADCreadings;
	Red /= Num_of_ADCreadings;

	Green = 4095-Green;
	Blue = 4095-Blue;
	Red = 4095-Red;

	// ********************* Autoranging ******************************
	/* Don't throw away saturated point because if it is a flash it will be missed completely.
	//if in low range and any channel is saturated, change ranges and throw away the point
	if(((Red >= 4095) || (Green >= 4095) || (Blue >= 4095)) && (P1OUT == 0x10)) {
		P1OUT = 0x17;
	}
	else {
		//if in high range, multiply counts by RANGE_MULT before tallying
		if(P1OUT == 0x17) {
			Red*=RANGE_MULT;
			Green*=RANGE_MULT;
			Blue*=RANGE_MULT;
		}
		//update the tally for each channel on a read
		redlong += Red;
		greenlong += Green;
		bluelong += Blue;
		counts++;
	}
	*/
	//update the tally for each channel on a read
		if(P1OUT == 0x17){ 		//if in high range, multiply counts by RANGE_MULT before tallying
			redlong += (((long int)Red)*RANGE_MULT);
			greenlong += (((long int)Green)*RANGE_MULT);
			bluelong += (((long int)Blue)*RANGE_MULT);
		}
		else{
			redlong += Red;
			greenlong += Green;
			bluelong += Blue;
		}
		counts++;

	//if in low range and near saturation, change to high range
	if(((Red >= 4000) || (Green >= 4000) || (Blue >= 4000)) && (P1OUT == 0x10)) {
		P1OUT = 0x17;
	}

	//if in high range and below threshold, keep track of how long below threshold or switch to low if it's been low three in a row
	if((Red < 75) && (Green < 75) && (Blue < 75) && (P1OUT == 0x17)) {  // 75*RANGE_MULT = 3000
		modecount++;
		if(modecount >= 3) {
			P1OUT = 0x10;
			modecount = 0;
		}
	}
	else if(P1OUT == 0x17) {
		modecount = 0;
	}
}

//set values to save to the EEPROM
void logRGB() {
	redlong /= counts;
	greenlong /= counts;
	bluelong /= counts;
	if(redlong>0xFEFF | greenlong>0xFEFF | bluelong>0xFEFF){ 	// 0xFEFF = 65279
		rangeFlag = 1;
		Red = (unsigned int)(redlong/10);
		Green = (unsigned int)(greenlong/10);
		Blue = (unsigned int)(bluelong/10);
	}
	else{
		rangeFlag = 0;
		Red = (unsigned int)redlong;
		Green = (unsigned int)greenlong;
		Blue = (unsigned int)bluelong;
	}
}

//clear variables after log
void RGBreset() {
	Red = 0;
	Green = 0;
	Blue = 0;
	redlong = 0;
	greenlong = 0;
	bluelong = 0;
	counts = 0;
}

// Read battery voltage
int readBatVolt(void){
	ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
	ADC12CTL1 = ADC12SHP;                     // Use sampling timer
	ADC12MCTL0 = (ADC12SREF_6 + ADC12INCH_11); 	// Vr+ = VeRef+ and Vr- = AVss
	ADC12IE = 0x0001;                         // Enable ADC12IFG.0
	ADC12CTL0 |= ADC12ENC;                    // Enable conversions
	ADC12CTL0 |= ADC12SC;                     // Start convn - software trigger

	__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, Enable interrupts
	__no_operation();

	return (int)A11results[0];
}
#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  // static unsigned int index = 0;

  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                          // Vector  0:  No interrupt
  case  2: break;                          // Vector  2:  ADC overflow
  case  4: break;                          // Vector  4:  ADC timing overflow
  case  6:		                           // Vector  6:  ADC12IFG0
  	  A11results[0] = ADC12MEM0;	       // Move A0 results, IFG is cleared
  	  ADC12CTL0 &= ~ADC12ENC;               // disable conversions
  	  ADC12CTL0 &= ~ADC12SC;                // Stop convn - software trigger
  	  ADC12IE &= ~0x0001;                   // disable ADC12IFG.0
  	  ADC12CTL0 &= ~ADC12ON; 				// Turn off ADC
  	  __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
  	break;
  case  8: break;                          // Vector  8:  ADC12IFG1
  case 10: 		                           // Vector 10:  ADC12IFG2
	    A0results[index] = ADC12MEM0;           // Move A0 results, IFG is cleared
	    A1results[index] = ADC12MEM1;           // Move A1 results, IFG is cleared
	    A2results[index] = ADC12MEM2;           // Move A2 results, IFG is cleared
	    index++;                                // Increment results index, modulo; Set Breakpoint1 here

	    if (index == Num_of_ADCreadings - 1) ADC12CTL0 &= ~ADC12ENC; // disable conversions (will stop at end of sequence)
	    if (index == Num_of_ADCreadings)
	    {
	    	ADC12CTL0 &= ~ADC12SC;                     // Stop convn - software trigger
	    	ADC12IE &= ~0x04;                          // disable ADC12IFG.2
			// ADC12CTL0 &= ~ADC12ENC;                    // disable conversions
			ADC12CTL0 &= ~ADC12ON; 						// Turn off ADC
			__bic_SR_register_on_exit(LPM0_bits);      // Exit LPM0
	    }
	  break;
  case 12: break; 							// Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                 		    // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}

