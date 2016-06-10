/*
 * os_state.c
 *
 *  Created on: 27 May 2016
 *      Author: Kristan Edwards
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


/*
 * Define states for each variable
 */
#define AC 0
#define DC 1
#define AUTO 0
#define NORMAL 1
#define SINGLE 2


/*
 * Initialises entire state of Oscilloscope
 */

uint8_t Coupling = AC;
uint8_t VerticalRange; 	// NC
uint8_t HorizontalRange;// NC
uint8_t TriggerMode = SINGLE;


void os_state_init(void) {
	;
}

void os_update_state(void){
	;
}


