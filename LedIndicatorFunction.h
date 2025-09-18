/*
 * LedIndicatorFunction.h
 *
 * Created: 10.08.2020 11:05:32
 *  Author: Анализатор спектра
 */ 


#ifndef LEDINDICATORFUNCTION_H_
#define LEDINDICATORFUNCTION_H_

#include "config.h"
#include <avr/interrupt.h>

typedef struct
{
	unsigned int dig_code[5]	;	
} Digit_code;


#define LED_DRIVER_DDR							DDRA
#define LED_DRIVER_PORT							PORTA
#define ANODE_DDR								DDRC
#define ANODE_PORT								PORTC
#define MBI_CLK									1
#define MBI_LE									2
#define MBI_OE									3
#define MBI_SDI_H								0
#define MBI_SDI_L								4
#define LED										7
#define VCC_BTN									5
#define DEVICES_NUMBER							MAX_DEVICES
#define TIM0_FREQENCY							244
#define TIM0_PRESCALER							1024
#define OCR0_register							((F_CPU/TIM0_PRESCALER)/TIM0_FREQENCY) - 1
#define TIM2_FREQENCY							500000
#define TIM2_PRESCALER							1
#define OCR2_register							((F_CPU/TIM2_PRESCALER)/TIM2_FREQENCY) - 1
#define TEN_SECONDS_TOP							TIM0_FREQENCY*10
#define ONE_SECOND_TOP							TIM0_FREQENCY
#define LED_PERIOD_PARTS_MAX_NUMBER				16
#define LED_PERIOD_PARTS_IN_SECOND				2

unsigned char temp_digits[4]	;
unsigned int output_high, output_low	;
Digit_code led_codes[DEVICES_NUMBER]	;
unsigned int letters[DEVICES_NUMBER]	;
unsigned char dev_num, dig_num	;
volatile unsigned int ten_seconds_counter, one_second_counter, led_counter 	;
volatile unsigned char ten_sec_flag, one_sec_flag, led_flag, out_flag, in_flag	;
unsigned int top_led, led_pulse_duration, led_pulse_period	;

void GetDisplayCode(Digit_code * code,unsigned int * temperature, unsigned char sensors_num)	;
void Init_timers(void)	;
void Init_mbi_ports(void)	;
void Timer2_ON(void)	;
void Timer2_OFF(void)	;
void Change_digit(unsigned char sensors_num)	;
void MBISignalGenerator(void)	;

#endif /* LEDINDICATORFUNCTION_H_ */
