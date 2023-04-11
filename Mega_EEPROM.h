/*! \file ********************************************************************
*
*
* \li File:               Mega_EEPROM.h
*
* \li Supported devices:  All AVRs.
*
*
* \li Description:        Header file for Mega_EEPROM.c
*
*                         $Revision: 1.7 $
*                         $Date: Friday, August 21, 2020 
****************************************************************************/

#ifndef MEGA_EEPROM_H_
#define MEGA_EEPROM_H_

#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>

#define EEPROM_READ_SUCCESFUL		0xFF
#define EEPROM_READ_ERROR			0
#define WRITE_ID_STRING				0x1
#define WRITE_LETTER_STRING			0x2

void Write_byte_mega_eeprom(uint16_t address, uint8_t eebyte)	;
void Write_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)	;
uint8_t Read_byte_mega_eeprom(uint16_t address)	;
void Read_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)	;
//функции записи и чтения с двойным резервированием
void Write_info_byte_mega_eeprom(uint16_t address, uint8_t eebyte)	;
uint8_t Read_info_byte_mega_eeprom(uint16_t address, uint8_t* eebyte)	;
void Write_reserved_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)	;
uint8_t Read_reserved_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)	;

#endif
