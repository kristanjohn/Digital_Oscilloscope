/*
 * os_dac.h
 *
 *  Created on: 3 May 2016
 *      Author: Kristan Edwards
 */

#ifndef OS_DAC_H_
#define OS_DAC_H_




void os_dac_init(void);
void os_dac_set_value(uint8_t value);
void os_dac_wave(uint8_t type, int frequency, uint8_t amplitude);
void os_dac_timer2A(void);

void os_dac_FuncOn(void);
void os_dac_FuncOff(void);
void os_dac_FreqUp(void);
void os_dac_FreqDown(void);
void os_dac_WaveType(uint8_t wave);
void os_dac_AmplitudeUp(void);
void os_dac_AmplitudeDown(void);
void CheckDac(void);



#endif /* OS_DAC_H_ */
