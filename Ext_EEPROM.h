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
*                         $Date: Friday, August 27, 2020 
****************************************************************************/

#ifndef EXT_EEPROM_H_
#define EXT_EEPROM_H_

//#define		F_CPU					1000000UL
#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define READ_COMMAND				0x03
#define WRITE_COMMAND				0x02
#define WREN_COMMAND				0x06
#define WRDI_COMMAND				0x04
#define RDSR_COMMAND				0x05
#define WRSR_COMMAND				0x01
#define PE_COMMAND					0x42
#define SE_COMMAND					0xD8
#define CE_COMMAND					0xC7


void Write_string_ext_eeprom(uint32_t address, uint8_t *eestring, uint8_t number)	;
void Read_string_ext_eeprom(uint32_t address, uint8_t *eestring, uint8_t number)	;
uint8_t Read_status_reg_eeprom()	;
void Write_status_reg_eeprom(uint8_t status)	;
void EEPROM_page_erase(uint32_t address)	;
void EEPROM_sector_erase(uint32_t address)	;
void EEPROM_chip_erase(void)	;

#endif
