#ifndef ADXL_H_
#define ADXL_H_

extern int Activity;

void setupADXL(void);
void stopADXL();
void readADXL();
void findActivityArray();
void findActivity();
void ADXLreset();

#endif /* ADXL_H_ */
