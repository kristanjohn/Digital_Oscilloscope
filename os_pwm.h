/*
 * os_pwm.h
 *
 *  Created on: 1 May 2016
 *      Author: Kristan Edwards
 */

#ifndef OS_PWM_H_
#define OS_PWM_H_

void os_pwm_init(void);
void os_pwm_set_funcgen(uint16_t duty_cycle); // Defines the DC offset
void os_pwm_set_chan0(uint16_t duty_cycle);
void os_pwm_set_chan1(uint16_t duty_cycle);
void os_pwm_FuncGenUp(void);
void os_pwm_FuncGenDown(void);

#endif /* OS_PWM_H_ */
