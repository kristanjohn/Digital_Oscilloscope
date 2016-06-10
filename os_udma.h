/*
 * os_udma.h
 *
 *  Created on: 27 Apr 2016
 *      Author: Kristan Edwards
 */

#ifndef OS_UDMA_H_
#define OS_UDMA_H_



void os_udma_init(void);				// Initialise uDMA
void uDMAErrorHandler(void);			// Handle Errors
void ADCseq0Handler(void);				// ADC0 Interrupt Handler
void ADC1seq0Handler(void);				// ADC1 Interrupt Handler
uint8_t os_udma_samples_ready(void);	// Flag to check ADC0
uint8_t os_udma_samples_ready1(void);	// Flag to check ADC1
uint8_t * os_udma_get_samples(void);	// Get ADC0 Samples
uint8_t * os_udma_get_samples1(void);	// Get ADC1 Samples
void os_udma_ForceTrigger(void);		// Set a Force Trigger
uint8_t os_udma_GetBitMode(void);		// Get Bit Mode
void os_udma_SampleMode(uint8_t mode);	// Set Bit Mode
uint16_t os_udma_GetTrigPos(void);		// Get Trigger Position
void os_udma_SetTriggerType(uint8_t cond);	// Set Trigger Type
void os_udma_ThresholdUp(void);			// Inc. Threshold
void os_udma_ThresholdDown(void);		// Dec.	Threshold
void os_udma_ClearTrigger(void);		// Clear Trigger
uint8_t os_udma_samples_BothReady(void);

#endif /* OS_UDMA_H_ */
