/*
 * os_pwm.c
 *
 * PWM used for DC offset
 * Change of DC offset is explicitly set by the user
 *
 *  Created on: 1 May 2016
 *      Author: Kristan Edwards
 */
#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c1294ncpdt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"


#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Clock.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
//#include <ti/drivers/PWM.h>

#include <driverlib/pwm.h>
/* Example/Board Header files */
#include "Board.h"
#include "os_pwm.h"

// Local Defines
#define OS_PWM_PERIOD 65535 //16 bit (Spec min is 10-bit)

uint8_t DutyCyclePeriod;
volatile uint8_t FuncGenVoltage = 0;

/*
 * @brief Set PWM voltage. 16bit value is
 * @params duty_cycle - 16bit comparator value of 3.3 V can be directly converted to volts
 * @retval None
 */
void os_pwm_set_funcgen(uint16_t duty_cycle) {

	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, duty_cycle);

}

/*
 * @brief Set PWM on PF1 for Channel 0
 */
void os_pwm_set_chan0(uint16_t duty_cycle) {
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, duty_cycle);
}

/*
 * @brief Set PWM on PF2 for Channel 1
 */
void os_pwm_set_chan1(uint16_t duty_cycle) {
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, duty_cycle);
}

/* ------------- USER CONFIGURATIONS ------------- */
void os_pwm_FuncGenUp(void) {

	if ((FuncGenVoltage + 3) <= 3300) {
		FuncGenVoltage += 3;
		os_pwm_set_funcgen(FuncGenVoltage * 0xFFFF / 3300);
	}
}

void os_pwm_FuncGenDown(void) {
	if ((FuncGenVoltage - 3) > 0) {
		FuncGenVoltage -= 3;
		os_pwm_set_funcgen(FuncGenVoltage * 0xFFFF / 3300);
	}
}

/* ------------------------------------------ */

/*
 * @brief Initialise PWM on pin
 *        PWM connected to PG0, PF1, PF2
 */
void os_pwm_init(void) {

	/* --- Create PWM on PG0 ---- */

	SysCtlPeripheralEnable( SYSCTL_PERIPH_PWM0 );
	// Check if the peripheral access is enabled.
	//
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0)){}

	SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOG );
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOG)){}

	SysCtlDelay(10);

	PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_1); 	// Set clock for PWM

	// Configure Pin
	GPIOPinConfigure(GPIO_PG0_M0PWM4);
	GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

	// Configure the PWM generator for count down mode with immediate updates to the parameters
	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN  | PWM_GEN_MODE_NO_SYNC);

	// Set the period. For a 250 MHz clock this translates to 65 535 clock ticks.
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, OS_PWM_PERIOD);

	// Set the pulse width of PWM0
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 1);

	// Enable the outputs
	PWMOutputState(PWM0_BASE, (PWM_OUT_4_BIT), true);

	PWMGenEnable(PWM0_BASE, PWM_GEN_2);

	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 1);

	/* --- Create PWM on PF1 --- */
	SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOF );
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)){}
	SysCtlDelay(10);
	// Configure Pin
	GPIOPinConfigure(GPIO_PF1_M0PWM1);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN  | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, OS_PWM_PERIOD);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 1);
	PWMOutputState(PWM0_BASE, (PWM_OUT_1_BIT), true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);

	/* --- Create PWM on PF2 --- */
	GPIOPinConfigure(GPIO_PF2_M0PWM2);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN  | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, OS_PWM_PERIOD);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 1);
	PWMOutputState(PWM0_BASE, (PWM_OUT_2_BIT), true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_1);


	/* --- Set initial state --- */
	os_pwm_set_funcgen(1);
	os_pwm_set_chan0(1);
	os_pwm_set_chan1(1);

}
