//***************************************************************************
//
//  Author(s)...: SW   
//
//  Target(s)...: ATMega16
//
//  Compiler....: GCC
//
//  Description.: Main functions for thermometer
//
//  Data........: 1.11.20 
//
//***************************************************************************
#ifndef GLOBAL_H
#define GLOBAL_H

#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "OWIHighLevelFunctions.h"
#include "OWIBitFunctions.h"
#include "common_files/OWIcrc.h"
#include "LedIndicatorFunction.h"
#include "twim.h"
#include "Mega_EEPROM.h"
#include "Ext_EEPROM.h"
#include "spi.h"
#include "usart.h"

#define SPI_PORTX   PORTB
#define SPI_DDRX    DDRB

#define SPI_MISO   6
#define SPI_MOSI   5
#define SPI_SCK    7
#define SPI_SS     4

//коды ошибок для функции чтения температуры
#define READ_SUCCESSFUL   0x00
#define READ_CRC_ERROR    0x01

#define SEARCH_SENSORS 0x00
#define SENSORS_FOUND 0xff

#define SENSOR_ID_SIZE 8
#define TEMPERATURE_VALUE_SIZE 2
#define LETTER_CODE_SIZE 2
#define SENSOR_INFO_SIZE (SENSOR_ID_SIZE + LETTER_CODE_SIZE + TEMPERATURE_VALUE_SIZE)

#define BRIGHTNESS_ARRAY_SIZE 16

//прототипы функций
void Init_peripheral(void)	;
void Timer1_Init(void)	;
void ADC_Init(void)	;
void Ports_Init(void)	;
unsigned char DS18B20_ReadTemperature(unsigned char bus, unsigned char * id, unsigned int* temperature);
void ID_Registration(unsigned char sensors_num, unsigned char crcFlag, uint8_t *local_id, OWI_device *allDevices)	;
void Brightness_measure(uint8_t *brightness_l, uint8_t *brightness_h)	;
void Convert4_to_1(uint8_t *data, uint32_t *dword)	;
void Convert1_to_4(uint8_t *data, uint32_t *dword)	;
void Check_memory_space(uint16_t *top_led)	;
void GetLetter(unsigned int * letter, uint8_t * local_id, unsigned char sensors_num)	;
void WriteTemperatureTime(uint8_t *local_id, uint8_t sensors_num, unsigned int *temperature, uint8_t *flag)	;
void SendTime(uint8_t SizePkt, uint8_t command)	;
void WriteTime(uint8_t SizePkt, uint8_t *data)	;
void SendEEPROMBytes(uint8_t SizePkt, uint8_t *data)	;
void Send25AA1024Bytes(uint8_t SizePkt, uint8_t *data)	;
void SendCurrentAddress(void)	;
void ReadAllTelemetry(void)	;
void ReadFullTelemetryByPackets(uint8_t *data)	;
void SendServiceData(void)	;
void SendDataPacket(unsigned char type)	;
void CheckAllDataSended(void)	;
void WriteLetterCodeToMemory(uint8_t *data, uint8_t * local_id)	;
void SendSensorsTemperature(unsigned char sensors_num, unsigned int *temperature, uint8_t *local_id, OWI_device *allDevices)	;
void SendBrightnessValue(uint8_t brightness_l, uint8_t brightness_h)	;

#endif //GLOBAL_H