#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_adc.h"
#include "inc/hw_types.h"
#include "inc/hw_udma.h"
#include "inc/hw_emac.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
//#include "drivers/pinout.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "driverlib/udma.h"
#include "driverlib/emac.h"
#include "utils/uartstdio.h"
//#include <xdc/runtime/System.h>
#include "driverlib/rom.h"
#include "os_udma.h"
#include "os_PacketTypes.h"




//*****************************************************************************
//
// Global variables for controlling uDMA and ADCs
//
//*****************************************************************************

#define BIT_MODE8 8		// Define bit mode
#define BIT_MODE12 12

#define RISING_EDGE 0	// Define Trigger Conditions
#define FALLING_EDGE 1
#define LEVEL 3

uint8_t Bit_Mode = BIT_MODE12;			// Bit Mode Global
uint16_t Threshold = 2048;				// Threshold Global
uint8_t Trigger_Condition = RISING_EDGE;// Condition
volatile uint8_t Triggered = 0;			// Set when the second trigger condition is met
volatile uint8_t Trigger_Part1 = 0;		// Sets the first trigger condition
volatile uint16_t Trigger_Position = 0;	// Stores the index of the trigger within the ADC buffers

uint8_t udmaControlTable[1024] __attribute__ ((aligned(1024)));
uint8_t Adc0Buffer[51000] = {0}; 	// ADC 0 Samples
uint8_t Adc1Buffer[51000] = {0};	// ADC 1 Samples

volatile uint32_t Adc0_buf_index = 0;		// ADC0 buffer
volatile uint32_t Adc1_buf_index = 0;		// ADC1 buffer

// Temporary Buffers
uint16_t PrimPing0[8] = {0}; // ADC0 is ping
uint16_t PrimPong1[8] = {0}; // ADC1 is pong
uint16_t AltPing0[8] = {0};
uint16_t AltPong1[8] = {0};

#define PRIM 0
#define ALT 1

volatile uint8_t PingIndex = PRIM;
volatile uint8_t PongIndex = PRIM;

volatile uint16_t Samples_Acquired = 0; // Only use 1 of these
volatile uint16_t Samples_Acquired1 = 0;

//DMA bus error count
uint32_t g_ui32uDMAErrCount = 0;

#define ADC_SAMPLE_BUF_SIZE 8

/* Protypes */
extern void InterruptUser(void);


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
	while (1);
}
#endif

volatile uint8_t Samples_Ready = 0;		// State when an acquisition is complete
volatile uint8_t Samples_Ready1 = 0;

/* -------------- Configurations ------------- */
/*
 *@brief 	Change Sample Mode
 */
void os_udma_SampleMode(uint8_t mode) {
	Bit_Mode = mode;
}

/*
 * @brief  	Set Trigger mode
 */
void os_udma_SetTriggerType(uint8_t cond) {
	if (cond == TYPE_TRIG_TYPE_RISE) {
		Trigger_Condition = RISING_EDGE;
	}
	else if (cond == TYPE_TRIG_TYPE_FALL) {
		Trigger_Condition = FALLING_EDGE;
	}
	else if (cond == TYPE_TRIG_TYPE_LVL) {
		Trigger_Condition = LEVEL;
	}
}

/*
 * @brief 	Trigger Threshold Up
 */
void os_udma_ThresholdUp(void) {
	/* Threshold has 20 possible settings */
	if ((Threshold - 4095/20) >= 0) {
		Threshold = Threshold - 4095/20;
	}
}

/*
 * @brief 	Trigger Threshold Down
 */
void os_udma_ThresholdDown(void) {
	if ((Threshold + 4095/20) <= 4095) {
		Threshold = Threshold + 4095/20;
	}
}

/*
 * @brief 	Get Trigger Packet
 */
uint16_t os_udma_GetTrigPos(void) {
	if (Bit_Mode == 12) {
		return (Trigger_Position * 8 / 12);
	}
	return Trigger_Position;
}

/*
 * @breif 	Return current bit mode 12 or 8
 */
uint8_t os_udma_GetBitMode(void) {
	return Bit_Mode;
}

/*
 * @brief 	Get ADC0 sampling buffer
 */
uint16_t * get_ADC_buf(void) {

	/* Check what the previous buffer was and return the other one */
	if (PingIndex == PRIM) {
		PingIndex = ALT;
		return &AltPing0[0];
	} else {
		PingIndex = PRIM;
		return &PrimPing0[0];
	}
}

/*
 * @brief 	Get ADC0 sampling buffer
 */
uint16_t * get_ADC1_buf(void) {

	/* Check what the previous buffer was and return the other one */
	if (PongIndex == PRIM) {
		PongIndex = ALT;
		return &AltPong1[0];
	} else {
		PongIndex = PRIM;
		return &PrimPong1[0];
	}
}


/*
 * @brief 	Initialise the ADC peripheral with uDMA
 */
void ConfigureADC(void)
{

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);	// Use PE0
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1);	// Set Pin type to Analog IN

    SysCtlDelay(10);

    ADCSequenceConfigure(ADC0_BASE, 0 /* SS0 */ , ADC_TRIGGER_PROCESSOR, 3 /* priority */);
    ADCSequenceConfigure(ADC0_BASE, 3 /* SS3 */ , ADC_TRIGGER_PROCESSOR, 0 /* priority */); // Doubt this is needed
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH3 ); // Channel 3 -> PE0
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH3 );
    ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH3 );
    ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH3 );
    ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH3 );
    ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH3 );
    ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH3 );
    ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH3 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 0);

    ADCSequenceConfigure(ADC1_BASE, 0 /* SS0 */ , ADC_TRIGGER_PROCESSOR, 3 /* priority */);
	ADCSequenceConfigure(ADC1_BASE, 3 /* SS3 */ , ADC_TRIGGER_PROCESSOR, 0 /* priority */); // Doubt this is needed
	ADCSequenceStepConfigure(ADC1_BASE, 0, 0, ADC_CTL_CH2 ); // Channel 3 -> PE1
	ADCSequenceStepConfigure(ADC1_BASE, 0, 1, ADC_CTL_CH2 );
	ADCSequenceStepConfigure(ADC1_BASE, 0, 2, ADC_CTL_CH2 );
	ADCSequenceStepConfigure(ADC1_BASE, 0, 3, ADC_CTL_CH2 );
	ADCSequenceStepConfigure(ADC1_BASE, 0, 4, ADC_CTL_CH2 );
	ADCSequenceStepConfigure(ADC1_BASE, 0, 5, ADC_CTL_CH2 );
	ADCSequenceStepConfigure(ADC1_BASE, 0, 6, ADC_CTL_CH2 );
	ADCSequenceStepConfigure(ADC1_BASE, 0, 7, ADC_CTL_CH2 | ADC_CTL_IE | ADC_CTL_END);
	ADCPhaseDelaySet(ADC1_BASE, ADC_PHASE_180);
	ADCSequenceEnable(ADC1_BASE, 0);
    //
    // Enable the uDMA controller.
    //
    uDMAEnable();

    //
    // Point at the control table to use for channel control structures.
    //
    uDMAControlBaseSet(udmaControlTable);

    uDMAChannelAssign(UDMA_CH14_ADC0_0); // Check the channel
    uDMAChannelAssign(UDMA_CH24_ADC1_0);
//    uDMAChannelAssign(UDMA_CH24_SSI1RX);

	ADCSequenceDMAEnable(ADC0_BASE, 0);
	ADCSequenceDMAEnable(ADC1_BASE, 0);

	// disable some bits
	uDMAChannelAttributeDisable(UDMA_CH14_ADC0_0, UDMA_ATTR_ALTSELECT /*start with ping-pong PRI side*/ |
		UDMA_ATTR_HIGH_PRIORITY /*low priority*/ | UDMA_ATTR_REQMASK /*unmask*/);
	// enable some bits
	uDMAChannelAttributeEnable(UDMA_CH14_ADC0_0, UDMA_ATTR_USEBURST /*only allow burst transfers*/);

	// disable some bits
	uDMAChannelAttributeDisable(UDMA_CH24_SSI1RX, UDMA_ATTR_ALTSELECT /*start with ping-pong PRI side*/ |
		UDMA_ATTR_HIGH_PRIORITY /*low priority*/ | UDMA_ATTR_REQMASK /*unmask*/);
	// enable some bits
	uDMAChannelAttributeEnable(UDMA_CH24_SSI1RX, UDMA_ATTR_USEBURST /*only allow burst transfers*/);


    // set dma params on PRI_ and ALT_SELECT
    uDMAChannelControlSet(UDMA_CH14_ADC0_0 | UDMA_PRI_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_128); //1024 or 128
    uDMAChannelControlSet(UDMA_CH14_ADC0_0 | UDMA_ALT_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_128);

    uDMAChannelEnable(UDMA_CH14_ADC0_0);

    uDMAChannelControlSet(UDMA_CH24_SSI1RX | UDMA_PRI_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_128); //1024 or 128
    uDMAChannelControlSet(UDMA_CH24_SSI1RX | UDMA_ALT_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_128);

    uDMAChannelEnable(UDMA_CH24_SSI1RX);


    IntEnable(INT_UDMAERR);
    IntEnable(INT_ADC0SS0);
    IntEnable(INT_ADC1SS0);

}

//*****************************************************************************
//
// The interrupt handler for uDMA errors.  This interrupt will occur if the
// uDMA encounters a bus error while trying to perform a transfer.  This
// handler just increments a counter if an error occurs.
//
//*****************************************************************************
void uDMAErrorHandler(void)
{
    uint32_t ui32Status;

    //
    // Check for uDMA error bit
    //
    ui32Status = uDMAErrorStatusGet();

    //
    // If there is a uDMA error, then clear the error and increment
    // the error counter.
    //
    if(ui32Status)
    {
        uDMAErrorStatusClear();
        g_ui32uDMAErrCount++;
    }
    while(1);
}


volatile uint32_t adc_complete_count = 0; 	// Counts the number of completed samples
volatile uint8_t level_above = 0;			// Used for Level Triggering
volatile uint8_t level_below = 0;

/*
 * @brief Check for Trigger. Test all conditions
 */
uint8_t check_trigger(uint16_t* buf) {
	/* Select trigger condition */
	switch (Trigger_Condition) {
		case (RISING_EDGE) :
			// First trigger condition set
			if (Trigger_Part1 == 1) {
				// Check for trigger above threshold
				if ((buf[0] > Threshold) ||	(buf[1] > Threshold)) {
					Triggered = 1;
					return 1;
				}
				return 0;
			// Check for value below threshold
			} else {
				if ((buf[0] < Threshold) || (buf[1] < Threshold)) {
					Trigger_Part1 = 1;
				}
				return 0;
			}
		case (FALLING_EDGE) :
			// Check if trigger condition set
			if (Trigger_Part1 == 1) {
				// Check for trigger below threshold
				if ((buf[0] < Threshold) || (buf[1] < Threshold)) {
					Triggered = 1;
					return 1;
				}
				return 0;
			// Check for value above threshold
			} else {
				if ((buf[0] > Threshold) || (buf[1] > Threshold)) {
					Trigger_Part1 = 1;
				}
				return 0;
			}
		case (LEVEL) :
			// Check if value below threshold
			if (level_above == 1) {
				if ((buf[0] < Threshold) || (buf[1] < Threshold)) {
					Triggered = 1;
					return 1;
				}
				return 0;
			// Check if value above threshold
			} else if (level_below == 1) {
				if ((buf[0] > Threshold) ||	(buf[1] > Threshold)) {
					Triggered = 1;
					return 1;
				}
				return 0;
			// Decide if above or below
			} else {
				// Value below level
				if ((buf[0] < Threshold) | (buf[1] < Threshold)) {
					level_below = 1;
				} else {
					level_above = 1;
				}
				return 0;
			}
		default:
			return 0;
	}
}

volatile uint8_t Force = 0;	// Set if force trigger is used to take extra samples
volatile uint8_t Force1 = 0;
/*
 * @brief 	Force Trigger
 */
void os_udma_ForceTrigger(void) {
	Triggered = 1;
	if (Bit_Mode == 12) {
		Trigger_Position = 25500;
	}
	Trigger_Position = 25500;
	Force = 1;
	Force1 = 1;
}

/*
 * @brief 	Clear Trigger
 */
void os_udma_ClearTrigger(void) {
	Triggered = 0;
}

/*
 * @brief Transfers Temp buffer to large ADC0 buffer
 * 		Works depending on whether 8 BIT or 12 BIT mode is selected
 * 		Compresses in 12BIT mode
 */
void TransferToADC0Buffer(uint16_t temp[8]) {
	// 12 bit mode
	if (Bit_Mode == BIT_MODE12) {
		// Compress
		Adc0Buffer[Adc0_buf_index++] = (temp[0] & 0x0FF); 				// LS 8bits of Sample 0
		Adc0Buffer[Adc0_buf_index++] = ((temp[0] & 0xF00) >> 8) |		// MS 4bits of Sample 0 &
											((temp[1] & 0x00F) << 4); 	// LS 4bits of Sample 1
		Adc0Buffer[Adc0_buf_index++] = (temp[1] & 0xFF0 >> 4);			// MS 8bits of Sample 1

		Adc0Buffer[Adc0_buf_index++] = (temp[2] & 0x0FF);		 		// LS 8bits of Sample 2
		Adc0Buffer[Adc0_buf_index++] = ((temp[2] & 0xF00) >> 8) |		// MS 4bits of Sample 2 &
											((temp[3] & 0x00F) << 4); 	// LS 4bits of Sample 3
		Adc0Buffer[Adc0_buf_index++] = ((temp[3] & 0xFF0) >> 4);		// MS 8bits of Sample 3

		Adc0Buffer[Adc0_buf_index++] = (temp[4] & 0x0FF); 				// LS 8bits of Sample 4
		Adc0Buffer[Adc0_buf_index++] = ((temp[4] & 0xF00) >> 8) |		// MS 4bits of Sample 4 &
											((temp[5] & 0x00F) << 4); 	// LS 4bits of Sample 5
		Adc0Buffer[Adc0_buf_index++] = ((temp[5] & 0xFF0) >> 4);		// MS 8bits of Sample 5

		Adc0Buffer[Adc0_buf_index++] = (temp[6] & 0x0FF);		 		// LS 8bits of Sample 6
		Adc0Buffer[Adc0_buf_index++] = ((temp[6] & 0xF00) >> 8) |		// MS 4bits of Sample 6 &
											((temp[7] & 0x00F) << 4); 	// LS 4bits of Sample 7
		Adc0Buffer[Adc0_buf_index++] = ((temp[7] & 0xFF0) >> 4);		// MS 8bits of Sample 7

	// Don't compress. 8 bit mode
	} else {
		Adc0Buffer[Adc0_buf_index++] = ((temp[0] & 0xFF0) >> 4);		// MS 8bits
		Adc0Buffer[Adc0_buf_index++] = ((temp[1] & 0xFF0) >> 4);
		Adc0Buffer[Adc0_buf_index++] = ((temp[2] & 0xFF0) >> 4);
		Adc0Buffer[Adc0_buf_index++] = ((temp[3] & 0xFF0) >> 4);
		Adc0Buffer[Adc0_buf_index++] = ((temp[4] & 0xFF0) >> 4);
		Adc0Buffer[Adc0_buf_index++] = ((temp[5] & 0xFF0) >> 4);
		Adc0Buffer[Adc0_buf_index++] = ((temp[6] & 0xFF0) >> 4);
		Adc0Buffer[Adc0_buf_index++] = ((temp[7] & 0xFF0) >> 4);
	}
}

/*
 * @brief Transfers Temp buffer to large ADC1 buffer
 * 		Works depending on whether 8 BIT or 12 BIT mode is selected
 * 		Compresses in 12BIT mode
 */
void TransferToADC1Buffer(uint16_t temp[8]) {
	// 12 bit mode
	if (Bit_Mode == BIT_MODE12) {
		// Compress
		Adc1Buffer[Adc1_buf_index++] = (temp[0] & 0x0FF); 				// LS 8bits of Sample 0
		Adc1Buffer[Adc1_buf_index++] = ((temp[0] & 0xF00) >> 8) |		// MS 4bits of Sample 0 &
											((temp[1] & 0x00F) << 4); 	// LS 4bits of Sample 1
		Adc1Buffer[Adc1_buf_index++] = ((temp[1] & 0xFF0) >> 4);		// MS 8bits of Sample 1

		Adc1Buffer[Adc1_buf_index++] = (temp[2] & 0x0FF); 				// LS 8bits of Sample 2
		Adc1Buffer[Adc1_buf_index++] = ((temp[2] & 0xF00) >> 8) |		// MS 4bits of Sample 2 &
											((temp[3] & 0x00F) << 4); 	// LS 4bits of Sample 3
		Adc1Buffer[Adc1_buf_index++] = ((temp[3] & 0xFF0) >> 4);		// MS 8bits of Sample 3

		Adc1Buffer[Adc1_buf_index++] = (temp[4] & 0x0FF);		 		// LS 8bits of Sample 4
		Adc1Buffer[Adc1_buf_index++] = ((temp[4] & 0xF00) >> 8) |		// MS 4bits of Sample 4 &
											((temp[5] & 0x00F) << 4); 	// LS 4bits of Sample 5
		Adc1Buffer[Adc1_buf_index++] = ((temp[5] & 0xFF0) >> 4);		// MS 8bits of Sample 5

		Adc1Buffer[Adc1_buf_index++] = (temp[6] & 0x0FF); 				// LS 8bits of Sample 6
		Adc1Buffer[Adc1_buf_index++] = ((temp[6] & 0xF00) >> 8) |		// MS 4bits of Sample 6 &
											((temp[7] & 0x00F) << 4); 	// LS 4bits of Sample 7
		Adc1Buffer[Adc1_buf_index++] = ((temp[7] & 0xFF0) >> 4);		// MS 8bits of Sample 7

	// Don't compress. 8 bit mode
	} else {
		Adc1Buffer[Adc1_buf_index++] = ((temp[0] & 0xFF0) >> 4);	// MS 8bits
		Adc1Buffer[Adc1_buf_index++] = ((temp[1] & 0xFF0) >> 4);
		Adc1Buffer[Adc1_buf_index++] = ((temp[2] & 0xFF0) >> 4);
		Adc1Buffer[Adc1_buf_index++] = ((temp[3] & 0xFF0) >> 4);
		Adc1Buffer[Adc1_buf_index++] = ((temp[4] & 0xFF0) >> 4);
		Adc1Buffer[Adc1_buf_index++] = ((temp[5] & 0xFF0) >> 4);
		Adc1Buffer[Adc1_buf_index++] = ((temp[6] & 0xFF0) >> 4);
		Adc1Buffer[Adc1_buf_index++] = ((temp[7] & 0xFF0) >> 4);
	}
}

/*
 * @brief Check the interrupt flag and complete the transfer of ADC
 */
void ADCprocess(uint32_t ch) {
	//uint8_t zeros[8] = {0,0,0,0,0,0,0,0};
	if ((((tDMAControlTable *) udmaControlTable)[ch].ui32Control & UDMA_CHCTL_XFERMODE_M) != UDMA_MODE_STOP) {
		return;
	}
	// Store the next buffer in the uDMA transfer descriptor
	// the ADC is read directly into the correct transmit buffer
	uDMAChannelTransferSet(ch, UDMA_MODE_PINGPONG, (void*) (ADC0_BASE + ADC_O_SSFIFO0), get_ADC_buf(), /* Get 8 samples */ 8);

	/* The address of the next buffer is &Adc0Buffer[8 * Adc0_buf_index++]; */
	// Fill the global buffer with the temporary one
	// 1. Find which buffer is full
	// 2. Check for trigger | or | count until complete
	// 3. Check 8 or 12bit Mode
	// 4. Transfer bytes
	if (PingIndex == PRIM) { // Then the ALT buffer is full
		if (Triggered == 0) {
			if (check_trigger(&AltPing0[0]) == 1) {
				Trigger_Position = Adc0_buf_index;	// Store trigger Positon
			}
		} else {
			Samples_Acquired += 8;
		}
		TransferToADC0Buffer(&AltPing0[0]);
		//memcpy(AltPing0, zeros, 8*sizeof(uint8_t));

	} else {
		if (Triggered == 0) {
			if (check_trigger(&PrimPing0[0]) == 1) {
				Trigger_Position = Adc0_buf_index;
			}
		} else {
			Samples_Acquired += 8;
		}
		TransferToADC0Buffer(&PrimPing0[0]);
		//memcpy(PrimPing0, zeros, 8*sizeof(uint8_t));
	}

	if (Adc0_buf_index >= 50999) {
		InterruptUser();
		Adc0_buf_index = 0;
	}
}

/*
 * @brief Check the interrupt flag and complete the transfer of ADC
 */
void ADC1process(uint32_t ch) {
	if ((((tDMAControlTable *) udmaControlTable)[ch].ui32Control & UDMA_CHCTL_XFERMODE_M) != UDMA_MODE_STOP) {
		return;
	}
	// Store the next buffer in the uDMA transfer descriptor
	// the ADC is read directly into the correct transmit buffer
	uDMAChannelTransferSet(ch, UDMA_MODE_PINGPONG, (void*) (ADC1_BASE + ADC_O_SSFIFO0), get_ADC1_buf(), /* Get 8 samples */ 8);

	if (PongIndex == PRIM) {
		TransferToADC1Buffer(&AltPong1[0]);
	} else {
		TransferToADC1Buffer(&PrimPong1[0]);
	}
	if (Triggered == 1) {
		Samples_Acquired1 += 8;
	}

	if (Adc1_buf_index >= 50999) {
		Adc1_buf_index = 0;
		// Stop to check user input
		InterruptUser();
	}
}

/*
 * @brief	returns true if there is a packet to send
 */
uint8_t os_udma_samples_BothReady(void) {
	if ((Samples_Ready == 1) && (Samples_Ready1 == 1)) {
		Samples_Ready = 0;
		Samples_Ready1 = 0;
		return 1;
	} else {
		return 0;
	}
}

/*
 * @brief	returns true if there is a packet to send
 */
uint8_t os_udma_samples_ready(void) {
	if (Samples_Ready == 1) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * @brief	returns true if there is a packet to send
 */
uint8_t os_udma_samples_ready1(void) {
	if (Samples_Ready1 == 1) {
		return 1;
	} else {
		return 0;
	}
}


/*
 * @brief 	Only use if samples are ready.
 * @retval	Returns a pointer to the samples
 */
uint8_t* os_udma_get_samples(void) {
	return Adc0Buffer;
}

uint8_t* os_udma_get_samples1(void) {
	return Adc1Buffer;
}
/*
 * @brief Call when all samples acuqired to reset
 */
void FinishAndResetADC1(void) {
	Adc1_buf_index = 0;
	Samples_Acquired1 = 0;
	Samples_Ready1 = 1;		// Notify it's complete
	Force1 = 0;
	ADCIntDisableEx(ADC1_BASE, ADC_INT_DMA_SS0);
	uDMAChannelDisable(UDMA_CH24_SSI1RX);
	ADCIntClearEx(ADC1_BASE, ADC_INT_DMA_SS0);

}

/*
 * @brief Call when all samples acuqired to reset
 */
void FinishAndResetADC0(void) {
	Adc0_buf_index = 0;
	Samples_Acquired = 0;
	Samples_Ready = 1;		// Notify it's complete
	Force = 0;
	Trigger_Part1 = 0;
	level_above = 0;
	level_below = 0;
	ADCIntDisableEx(ADC0_BASE, ADC_INT_DMA_SS0);
	uDMAChannelDisable(UDMA_CH14_ADC0_0);
	ADCIntClearEx(ADC0_BASE, ADC_INT_DMA_SS0);

}

/*
 * @brief Interrupt callback function when 8 ADC Samples from ADC1 are complete
 */
void ADC1seq0Handler(void)
{

	uint32_t ui32Status = ADCIntStatus(ADC1_BASE, 0, false);

	ADCIntClearEx(ADC1_BASE, ui32Status);

	ADCIntClearEx(ADC1_BASE, ADC_INT_DMA_SS0);

	ADC1process(UDMA_CH24_SSI1RX | UDMA_PRI_SELECT);
	ADC1process(UDMA_CH24_SSI1RX | UDMA_ALT_SELECT);

	/* Samples complete */
	if (Bit_Mode == BIT_MODE12) {
		if ((Samples_Acquired1 >= 17000) && (Force1 == 0)) {
			FinishAndResetADC1();
		}
		else if ((Samples_Acquired1 >= 34000) && (Force1 == 1)) {
			FinishAndResetADC1();
		} else {
			/* Keep going */
			uDMAChannelEnable(UDMA_CH24_SSI1RX);
			ADCProcessorTrigger(ADC1_BASE,0);
		}
	} else {
		if ((Samples_Acquired1 >= 25500) && (Force1 == 0)) {
			FinishAndResetADC1();
		}
		else if ((Samples_Acquired1 >= 51000) && (Force1 == 1)) {
			FinishAndResetADC1();
		} else {
			/* Keep going */
			uDMAChannelEnable(UDMA_CH24_SSI1RX);
			ADCProcessorTrigger(ADC1_BASE,0);
		}
	}
}

/*
 * @brief Interrupt callback function when 8 ADC Samples from ADCO are complete
 */
void ADCseq0Handler(void)
{

	uint32_t ui32Status = ADCIntStatus(ADC0_BASE, 0, false);

	/* Don't bother exiting from the interrupt */
	ADCIntClearEx(ADC0_BASE, ui32Status);
	ADCIntClearEx(ADC0_BASE, ADC_INT_DMA_SS0);

	ADCprocess(UDMA_CH14_ADC0_0 | UDMA_PRI_SELECT);
	ADCprocess(UDMA_CH14_ADC0_0 | UDMA_ALT_SELECT);

	/* Samples complete */
	// All samples have been acquired after ADC
	if (Bit_Mode == BIT_MODE12) {
		if ((Samples_Acquired >= 17000) && (Force == 0)) {
			FinishAndResetADC0();
		}
		else if ((Samples_Acquired >= 34000) && (Force == 1)) {
			FinishAndResetADC0();
		} else {
			/* Keep going */
			uDMAChannelEnable(UDMA_CH14_ADC0_0);
			ADCProcessorTrigger(ADC0_BASE,0);
		}
	} else {
		if ((Samples_Acquired >= 25500) && (Force == 0)) {
			FinishAndResetADC0();
		}
		else if ((Samples_Acquired >= 51000) && (Force == 1)) {
			FinishAndResetADC0();
		} else {
			/* Keep going */
			uDMAChannelEnable(UDMA_CH14_ADC0_0);
			ADCProcessorTrigger(ADC0_BASE,0);
		}
	}
}


/*
 * @brief Initialise uDMA for ADC transfers
 */
void os_udma_init()
{

    SysCtlDelay(5);

    ConfigureADC();

    /* The following lines are essential to start cycle */
    ADCIntClear(ADC0_BASE, 0);
    ADCIntClear(ADC1_BASE, 0);
    //ADCProcessorTrigger(ADC1_BASE,0);
}
