/*
 * lcd.h
 *
 *  Created on: 11 Apr 2016
 *      Author: Kristan
 */
#ifndef LCD_H_
#define LCD_H_

//*****************************************************************************
//
// Defines
//
//*****************************************************************************

// Port A
#define RD_PORT GPIO_PORTA_BASE
#define RD GPIO_PIN_7
// Port C
#define T_IRQ_PORT GPIO_PORTC_BASE
#define T_IRQ GPIO_PIN_7
// Port F
#define LED_PORT GPIO_PORTF_BASE
#define LED GPIO_PIN_3
// Port G
#define DB6_PORT GPIO_PORTG_BASE
#define DB6 GPIO_PIN_1
// Port H
#define DB12_PORT GPIO_PORTH_BASE
#define DB12 GPIO_PIN_0
#define DB13_PORT GPIO_PORTH_BASE
#define DB13 GPIO_PIN_1
#define WR_PORT GPIO_PORTH_BASE
#define WR GPIO_PIN_2
#define RS_PORT GPIO_PORTH_BASE
#define RS GPIO_PIN_3
// Port K
#define T_DIN_PORT GPIO_PORTK_BASE
#define T_DIN GPIO_PIN_2
#define T_CS_PORT GPIO_PORTK_BASE
#define T_CS GPIO_PIN_3
#define DB7_PORT GPIO_PORTK_BASE
#define DB7 GPIO_PIN_4
#define DB8_PORT GPIO_PORTK_BASE
#define DB8 GPIO_PIN_5
#define DB14_PORT GPIO_PORTK_BASE
#define DB14 GPIO_PIN_6
#define DB15_PORT GPIO_PORTK_BASE
#define DB15 GPIO_PIN_7
// Port L
#define DB2_PORT GPIO_PORTL_BASE
#define DB2 GPIO_PIN_0
#define DB3_PORT GPIO_PORTL_BASE
#define DB3 GPIO_PIN_1
#define DB4_PORT GPIO_PORTL_BASE
#define DB4 GPIO_PIN_2
#define DB5_PORT GPIO_PORTL_BASE
#define DB5 GPIO_PIN_3
#define DB0_PORT GPIO_PORTL_BASE
#define DB0 GPIO_PIN_4
#define DB1_PORT GPIO_PORTL_BASE
#define DB1 GPIO_PIN_5
// Port N
#define RST_PORT GPIO_PORTN_BASE
#define RST GPIO_PIN_2
#define CS_PORT GPIO_PORTN_BASE
#define CS GPIO_PIN_3
// Port P
#define T_DOUT_PORT GPIO_PORTP_BASE
#define T_DOUT GPIO_PIN_2
#define DB11_PORT GPIO_PORTP_BASE
#define DB11 GPIO_PIN_3
#define T_CLK_PORT GPIO_PORTP_BASE
#define T_CLK GPIO_PIN_4
#define DB9_PORT GPIO_PORTP_BASE
#define DB9 GPIO_PIN_5
// Port Q
#define DB10_PORT GPIO_PORTQ_BASE
#define DB10 GPIO_PIN_1


#define X_CONST 240
#define Y_CONST 320

#define PREC_TOUCH_CONST 10

#define PixSizeX	13.78
#define PixOffsX	411

#define PixSizeY	11.01
#define PixOffsY	378

/* LCD color */
#define WHITE          0xFFFF
#define BLACK          0x0000
#define BLUE           0x001F
#define BLUE2          0x051F
#define RED            0xF800
#define MAGENTA        0xF81F
#define GREEN          0x07E0
#define CYAN           0x7FFF
#define YELLOW         0xFFE0

/* Menu choices */
#define MAIN_MENU 1
#define RANGES_MENU 2
#define BACKLIGHT_MENU 3
#define FUNCTION_MENU 4
#define TRIGGER_MENU 5
#define BACKLIGHT_OFF 6

#define AC_COUPLING 0
#define DC_COUPLING 1

#define HOR_UP 0
#define HOR_DOWN 1
#define VER_UP 2
#define VER_DOWN 3

#define FUNCTION_OFF 0
#define FUNCTION_ON 1

#define TRIGGER_AUTO 0
#define TRIGGER_NORMAL 1
#define TRIGGER_SINGLE 2

#define TRIGGER_ARMED 0
#define TRIGGER_TRIGGERED 1
#define TRIGGER_STOPPED 2

#define TRIGGER_RISING 0
#define TRIGGER_FALLING 1
#define TRIGGER_LEVEL 2

#define FUNCTION_WAVE_SINE 0
#define FUNCTION_WAVE_SQUARE 1
#define FUNCTION_WAVE_TRIANGLE 2
#define FUNCTION_WAVE_RAMP 3
#define FUNCTION_WAVE_RANDOM 4

#define LIGHT_LEVEL_OFF 0
#define LIGHT_LEVEL_1 1
#define LIGHT_LEVEL_2 2
#define LIGHT_LEVEL_3 3
#define LIGHT_LEVEL_4 4
#define LIGHT_LEVEL_5 5


/* Function definitions */
void writeCommand( uint16_t);
void writeData( uint16_t);
void writeCommandData( uint16_t, uint16_t);
void initLCD(void);
void SetXY( uint16_t, uint16_t, uint16_t, uint16_t);
void clearLCD(void);
void initTouch(void);
void touchWriteData(unsigned char);
uint16_t touchReadData(void);
void touchRead(void);
char touchDataAvailable(void);
uint16_t touchGetX(void);
uint16_t touchGetY(void);
void displaySetup(void);
void drawMainMenu(void);
void drawTriggerMenu(void);
void drawBacklightMenu(void);
void drawRangesMenu(void);
void drawFunctionMenu(void);
void changeTriggerType(uint8_t);
void changeTriggerMode(uint8_t);
void changeFunctionWave(uint8_t);
void changeFunctionOnOff(uint8_t);
void changeCoupling(uint8_t);
uint16_t touchMainMenu(uint16_t, uint16_t);
uint16_t touchTriggerMenu(uint16_t, uint16_t);
uint16_t touchRangesMenu(uint16_t, uint16_t);
uint16_t touchBacklightMenu(uint16_t, uint16_t);
uint16_t touchFunctionMenu(uint16_t, uint16_t);
void drawTriggerState(uint16_t);
void drawConnected(uint16_t);
void clearMain(void);
uint8_t lcd_check_flag(void);
void lcd_connected(void);

#endif /* LCD_H_ */
