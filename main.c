#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <stdio.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/GPIO.h>

/* NDK BSD support */
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

/* Example/Board Header file */
#include "Board.h"

#include "driverlib/adc.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"

#include "driverlib/gpio.h"

/* Custom Headers */
#include "os_udma.h"
#include "os_pwm.h"
#include "os_dac.h"
#include "os_PacketTypes.h"
#include "os_enet.h"
#include "lcd.h"
#include "os_State.h"

#define TCPPACKETSIZE 250
#define NUMTCPWORKERS 3
//volatile char Enet_buffer[TCPPACKETSIZE] = {0}; 	// Packet to be sent

/*************************************************
 * Pinouts
 * 	PE0 - ADC Channel 0
 * 	PE1 - ADC Channel 1
 * 	PG0 - PWM DC offset Function Generator
 * 	PM  - DAC
 * 	PF1 - PWM Channel 0 DC offset
 * 	PF2 - PWM Channel 1 DC offset
 *
 * 	LCD
 *	PF3 - PWM Brightness
 *
 *************************************************/


volatile uint8_t LcdInt = 0; // LCD Interupt Flag
int ClientFd;

/* State Variables for Oscilloscope Operation */
uint8_t Armed = DISARMED;
uint8_t TrigMode = SINGLE;

/*
 * @breif Start ADC sampling.
 * 	Warning : This will stop all other functionality until complete
 */
void adc_int_enable(void) {
	uDMAChannelEnable(UDMA_CH14_ADC0_0);		// Enables uDMA and Ints
	ADCProcessorTrigger(ADC0_BASE,0);
	ADCIntClear(ADC0_BASE, 0);
	ADCIntEnableEx(ADC0_BASE, ADC_INT_DMA_SS0);
	while(os_udma_samples_ready() == 0);
	uDMAChannelEnable(UDMA_CH24_SSI1RX);
	ADCProcessorTrigger(ADC1_BASE,0);
	ADCIntClear(ADC1_BASE, 0);
	ADCIntEnableEx(ADC1_BASE, ADC_INT_DMA_SS0);
	while(os_udma_samples_ready1() == 0);

}

#define LCD_SOURCE 1 // Define source
#define PC_SOURCE 2

/*
 * @brief Start functionality that matches incoming packet type
 * 			Input is taken from both source PC and LCD
 */
void ProcessPacket(uint8_t type, uint8_t source) {

	System_printf("TYPE: %x\n", type);
	System_flush();
	switch (type) {
	case (TYPE_CHAN_COUP_AC) :	// Not needed
		if (source == PC_SOURCE) {
			changeCoupling(AC_COUPLING);
		}
		return;
	case (TYPE_CHAN_COUP_DC) :	// Not needed
		if (source == PC_SOURCE) {
			changeCoupling(DC_COUPLING);
		}
		return;
	case (TYPE_VERT_RANGE_UP) : // Not needed
		return;
	case (TYPE_VERT_RANGE_DW) : // Not needed
		return;
	case (TYPE_HORZ_RANGE_UP) : // Not needed
		return;
	case (TYPE_HORZ_RANGE_DW) : // Not needed
		return;
	case (TYPE_TRIG_MODE_AUTO) :		// Change Trigger Mode
		TrigMode = AUTO;
		if (source == PC_SOURCE) {
			changeTriggerMode(TRIGGER_AUTO);
		}
		return;
	case (TYPE_TRIG_MODE_NORM) :
		TrigMode = NORMAL;
		if (source == PC_SOURCE) {
			changeTriggerMode(TRIGGER_NORMAL);
		}
		return;
	case (TYPE_TRIG_MODE_SING) :
		TrigMode = SINGLE;
		if (source == PC_SOURCE) {
			changeTriggerMode(TRIGGER_SINGLE);
		}
		return;
	case (TYPE_TRIG_TYPE_RISE) :
		os_udma_SetTriggerType(type);	// Set Trigger Types
		changeTriggerType(TRIGGER_RISING);
		return;
	case (TYPE_TRIG_TYPE_FALL) :
		os_udma_SetTriggerType(type);
		changeTriggerType(TRIGGER_FALLING);
		return;
	case (TYPE_TRIG_TYPE_LVL) :
		os_udma_SetTriggerType(type);
		changeTriggerType(TRIGGER_LEVEL);
		return;
	case (TYPE_TRIG_THRESH_UP) :		// Change Trigger Threshold
		os_udma_ThresholdUp();
		return;
	case (TYPE_TRIG_THRESH_DW) :
		os_udma_ThresholdDown();
		return;
	case (TYPE_FUNC_ON) :
		os_dac_FuncOn();			// Turn Function Generator On
		if (source == PC_SOURCE) {
			changeFunctionOnOff(1);
		}
		return;
	case (TYPE_FUNC_OFF) :
		os_dac_FuncOff();			// Turn Function Generator Off
		if (source == PC_SOURCE) {
			changeFunctionOnOff(0);
		}
		return;
	case (TYPE_FUNC_SQUARE) :		// Set the wave types
		os_dac_WaveType(type);
		changeFunctionWave(FUNCTION_WAVE_SQUARE);
		return;
	case (TYPE_FUNC_SINE) :
		os_dac_WaveType(type);
		changeFunctionWave(FUNCTION_WAVE_SINE);
		return;
	case (TYPE_FUNC_RAMP) :
		os_dac_WaveType(type);
		changeFunctionWave(FUNCTION_WAVE_RAMP);
		return;
	case (TYPE_FUNC_TRIG) :
		os_dac_WaveType(type);
		changeFunctionWave(FUNCTION_WAVE_TRIANGLE);
		return;
	case (TYPE_FUNC_RANDOM) :
		os_dac_WaveType(type);
		changeFunctionWave(FUNCTION_WAVE_RANDOM);
		return;
	case (TYPE_FUNC_OFFSET_UP) :	// Change Function Gen DC offset level
		os_pwm_FuncGenUp();
		return;
	case (TYPE_FUNC_OFFSET_DW) :
		os_pwm_FuncGenDown();
		return;
	case (TYPE_FUNC_FREQ_UP) :		// Change Frequency of wave
		os_dac_FreqUp();
		return;
	case (TYPE_FUNC_FREQ_DW) :		// Change Amplitude of wave
		os_dac_FreqDown();
		return;
	case (TYPE_FUNC_VOLT_UP) :
		os_dac_AmplitudeUp();
		return;
	case (TYPE_FUNC_VOLT_DW) :
		os_dac_AmplitudeDown();
		return;
	case (TYPE_TRIG_ON) :			// Activates Trig Mode (Auto/Norm/Single)
		drawTriggerState(TRIGGER_ARMED); // Display status on LCD
		Armed = ARMED;
		return;
	case (TYPE_TRIG_OFF) :			// Turn Trigger off
		drawTriggerState(TRIGGER_STOPPED);
		Armed = DISARMED;
		return;
	case (TYPE_TRIG_FORCE) :
		drawTriggerState(TRIGGER_TRIGGERED);
		os_udma_ForceTrigger();		// Force Trigger
		adc_int_enable(); 			// Acquire Samples Immediately
		return;
	case (TYPE_BIT_MODE_8) :		// Change Bit Mode
		os_udma_SampleMode(8);
		return;
	case (TYPE_BIT_MODE_12) :
		os_udma_SampleMode(12);
		return;
	}
}

volatile int current_menu = MAIN_MENU;
volatile int previous_menu = MAIN_MENU;

/*
 *@brief Touch Handler Interrupt
 */
void TouchHandler(void) {
	int i, j;
	//int status;
	//GPIOIntClear(GPIO_PORTC_BASE, GPIO_PIN_7);

	LcdInt = 1; // Set interrupt flag

	touchRead();
	i = touchGetX();
	j = touchGetY();

	if (i > 0 && j > 0 && i < 240 && j < 320) {
		switch(current_menu) {
		case MAIN_MENU:
			previous_menu = MAIN_MENU;
			current_menu = touchMainMenu(i, j);
			break;
		case TRIGGER_MENU:
			previous_menu = TRIGGER_MENU;
			current_menu = touchTriggerMenu(i, j);
			break;
		case FUNCTION_MENU:
			previous_menu = FUNCTION_MENU;
			current_menu = touchFunctionMenu(i, j);
			break;
		case RANGES_MENU:
			previous_menu = RANGES_MENU;
			current_menu = touchRangesMenu(i, j);
			break;
		case BACKLIGHT_MENU:
			previous_menu = BACKLIGHT_MENU;
			current_menu = touchBacklightMenu(i, j);
			break;
		case BACKLIGHT_OFF:
			GPIOPinWrite(LED_PORT, LED, LED);
			current_menu = previous_menu;
			SysCtlDelay(100000);
			break;
		}
	}
}


/*
 * @brief 	Fast Interrupt Routine to check for user input
 * 			Optimized for Function Generator
 */
void InterruptUser(void) {
	uint8_t lcdFlag = 0;
	uint8_t packet[2] = {0};
	// Check for LCD input
	if (touchDataAvailable() == 1) {
		TouchHandler();
		lcdFlag = lcd_check_flag();
		if (lcdFlag != 0) {
			// Process Flag
			ProcessPacket(lcdFlag, LCD_SOURCE);
			packet[0] = lcdFlag;
			// Send while in interrupt
			send(ClientFd, packet, 1, /* flags */0);
		}
	}
	// Check for incoming packets in interrupt
	if ((recv(ClientFd, (void *) packet, 1, MSG_DONTWAIT)) > 0) {
		// Packet received. Lookup packet type
		ProcessPacket(packet[0], PC_SOURCE);
	}
}

/*
 *  ======== tcpWorker ========
 *  Task to handle TCP connection. Can be multiple Tasks running
 *  this function. All send and receives must happen in this task.
 */
Void tcpWorker(int clientfd) {
    int  i = 0;
    uint8_t packet[TCPPACKETSIZE] = {0};
    char buf[] = {(char)TYPE_CHAN0_SAMP, 12}; // Send TYPE then Bit Mode
    uint8_t lcdFlag = 0;
    uint16_t trigPos = 0;
    ClientFd = clientfd;			// Make File descriptor global

    /* Finish handshake */
    send(clientfd, "ACK",  4, /* flags */0);
    //StartLcdTask();
    lcd_connected();


    /* Main loop - Interrupts have higher priority */
    while(1) {

		/* Check for received packets */
		if (recv(clientfd, (void *) packet, 1, MSG_DONTWAIT) > 0) {
			// Packet received. Lookup packet type
			ProcessPacket(packet[0], PC_SOURCE);
		}

    	/* Check if LCD event resulted in packet to be sent LcdInt*/
		if (touchDataAvailable() == 1) {
			// Update screen
			TouchHandler();
			// Check which flag was set and clear it
			lcdFlag = lcd_check_flag();

			if (lcdFlag != 0) {
				// Apply function
				ProcessPacket(lcdFlag, LCD_SOURCE);
				// Send updates
				packet[0] = lcdFlag;
				if (send(clientfd, packet, 1, 0) < 0) {
					System_printf("Error: send failed.\n");
				}
			}
		}

		/* Check if Armed */
		if (Armed == ARMED) {
			/* Check for Mode */
			if (TrigMode == SINGLE) {
				drawTriggerState(TRIGGER_ARMED);
				Armed = DISARMED;			// Disarm
				adc_int_enable();			// Wait for trigger
				drawTriggerState(TRIGGER_TRIGGERED);
				drawTriggerState(TRIGGER_STOPPED);
			}
			else if (TrigMode == NORMAL) {
				drawTriggerState(TRIGGER_ARMED);
				adc_int_enable();			// Wait for trigger
				drawTriggerState(TRIGGER_TRIGGERED);
			}
			else if (TrigMode == AUTO) {
				drawTriggerState(TRIGGER_ARMED);
				os_udma_ForceTrigger();		// Force Trigger
				adc_int_enable(); 			// Acquire Samples Immediately
				drawTriggerState(TRIGGER_TRIGGERED);
			}
		}

		/* Check if ADC0 ready to send  && Check if ADC1 ready to send*/
    	/* ------    1 Trigger will fire both ADCs  -------  */
		//System_printf("about to check samples\n");
		//System_flush();
		if (os_udma_samples_BothReady() == 1) {
			os_udma_ClearTrigger();

			/* Get Trigger location */
			trigPos = os_udma_GetTrigPos();
			/* Send Trigger Location */
			packet[0] = TYPE_TRIG_POS;
			packet[1] = trigPos & 0x0FF;
			packet[2] = (trigPos & 0xFF00) >> 8;
			if (send(clientfd, packet, 128, 0) < 0) {
				System_printf("Error: send failed.\n");
			}

			/* Send ADC0 Packet samples */
			buf[0] = (char)TYPE_CHAN0_SAMP; // Set type
			buf[1] = os_udma_GetBitMode();
			send(clientfd, buf, 2, 0); // Send type and size

			/* Send 51000 bytes in 204 packets of 250 bytes*/
			for (i = 0; i < 204; i++) {

				// Copy over samples for current packet
				memcpy(packet, os_udma_get_samples() + (i*TCPPACKETSIZE) , TCPPACKETSIZE * sizeof(uint8_t));

				// Send 256 byte packet
				if (send(clientfd, packet, TCPPACKETSIZE, 0) < 0) {
					System_printf("Error: send failed.\n");
				}
			}

			/* Send ADC1 Packet samples */
			buf[0] = (char) TYPE_CHAN1_SAMP; // Change type
			buf[1] = os_udma_GetBitMode();
			if (send(clientfd, buf, 2, 0) < 0) {; // Send type and size
				System_printf("Error: send failed.\n");
			}

			// Split samples and send in seperate TCP packets
			for (i = 0; i < 204; i++) {

				// Copy over samples for current packet
				memcpy(packet, os_udma_get_samples1() + (i*TCPPACKETSIZE) , TCPPACKETSIZE * sizeof(uint8_t));

				// Send packet
				if (send(clientfd, (const void *) packet, TCPPACKETSIZE, 0) < 0) {
					System_printf("Error: send failed.\n");
				}
			}
		}
		CheckDac();
    }

    //close(clientfd); // Never closes. Reset Required
}


/*
 *  ======== main ========
 */
int main(void)
{
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initEMAC();

    /* Initialise Hardware */
    os_udma_init();
    os_pwm_init();
    os_dac_init();

    displaySetup();
	GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_7);
	GPIOIntDisable(GPIO_PORTC_BASE, GPIO_PIN_7);
	GPIOIntClear(GPIO_PORTC_BASE, GPIO_PIN_7);

    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}

