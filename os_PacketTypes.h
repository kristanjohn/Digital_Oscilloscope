/*
 * os_PacketTypes.h
 *
 *  Created on: 26 May 2016
 *      Author: Kristan Edwards
 */

#ifndef OS_PACKETTYPES_H_
#define OS_PACKETTYPES_H_

#define TYPE_CHAN0_SAMP 	0x00	// Channel 0 Sample Type
#define TYPE_CHAN1_SAMP 	0x01	// Channel 1 Sample Type
#define TYPE_CHAN_COUP_AC 	0x02	// Coupling
#define TYPE_CHAN_COUP_DC	0x03
#define TYPE_VERT_RANGE_UP	0x04	// Ranges
#define TYPE_VERT_RANGE_DW	0x05
#define TYPE_HORZ_RANGE_UP	0x06
#define TYPE_HORZ_RANGE_DW	0x07
#define TYPE_TRIG_MODE_AUTO	0x08	// Trig Modes
#define TYPE_TRIG_MODE_NORM	0x09

#define TYPE_TRIG_MODE_SING	0x10
#define TYPE_TRIG_TYPE_RISE	0x11	// Trig Types
#define TYPE_TRIG_TYPE_FALL 0x12
#define TYPE_TRIG_TYPE_LVL	0x13
#define TYPE_TRIG_THRESH_UP	0x14	// Trig Threshold
#define TYPE_TRIG_THRESH_DW 0x15
#define TYPE_FUNC_ON		0x16	// Func Gen ON/OFF
#define TYPE_FUNC_OFF		0x17
#define TYPE_FUNC_SQUARE	0x18	// Func Gen Waveform
#define TYPE_FUNC_SINE		0x19

#define TYPE_FUNC_RAMP		0x20
#define TYPE_FUNC_TRIG		0x21
#define	TYPE_FUNC_RANDOM	0x22
#define TYPE_FUNC_OFFSET_UP	0x23	// Func Gen offset
#define TYPE_FUNC_OFFSET_DW	0x24
#define TYPE_FUNC_FREQ_UP	0x25	// Func Gen Freq
#define	TYPE_FUNC_FREQ_DW	0x26
#define TYPE_FUNC_VOLT_UP	0x27	// Func Pk-Pk Voltage
#define TYPE_FUNC_VOLT_DW	0x28
#define TYPE_TRIG_ON		0x29	// Trigger

#define TYPE_TRIG_OFF		0x30
#define TYPE_TRIG_FORCE 	0x31
#define TYPE_BIT_MODE_8		0x32	// Bit Modes
#define TYPE_BIT_MODE_12	0x33
#define TYPE_TRIG_POS		0x34	// Trigger Position (2byte Packet)
#define TYPE_TRIGGERED		0x35




#endif /* OS_PACKETTYPES_H_ */
