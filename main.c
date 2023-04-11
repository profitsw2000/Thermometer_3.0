/*
 * Thermometer_3.0.c
 *
 ��������...............
 �������� ��������� ���������� ����������� 3-�� ������.
 ������ ��������� ��������� ��������� �������:
 - ��������� ����� �������� ����������� �� ����� (�������� 1-Wire);
 - ��������� ����������� � ��������� ��������;
 - ��������� ���������� ���������� ����������� �� ������������ ��������������� �����������;
 - ��������� ����� � ������� DS1307 (�������� TWI);
 - ���������� ������ 10 ����� �������� ���������� � ���������� ������ 25��1024 (�������� SPI);
 - ���������� ������� ������������ � ������� ������������� � ���������� ������� �������� ������������ �����������;
 - ����� ���������� �� ��������� ��� ������������ ��������� bluetooth-������ � �������������� � ����� ���������� ���������� ������
 � ���������� 25��1024;
 - ������������ ����������� � �������� ������� ����������� ��������� UART, � ��� ����� ������� ������ �� ���������� ������ 25��1024;
 *
 * Created: 01.11.2020 13:42:48
 * Author : Asus
 */ 

#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "usart.h"
#include "twim.h"
#include "Mega_EEPROM.h"
#include "Ext_EEPROM.h"
#include "spi.h"
#include "LedIndicatorFunction.h"
#include "OWIHighLevelFunctions.h"
#include "OWIBitFunctions.h"
#include "OWIPolled.h"
#include "common_files/OWIcrc.h"
#include "compilers.h"
#include "global.h"

void Parse_UART(void)	;
void RS_Decode(uint8_t *data, uint8_t comand, uint8_t SizePkt)	;

OWI_device allDevices[MAX_DEVICES]	;
uint8_t local_id[MAX_DEVICES]	;
unsigned char crcFlag = 0;
unsigned char sensors_num = 0;
unsigned int temperature[MAX_DEVICES]	;
unsigned char brightness_l, brightness_h	;
unsigned char temp	;
uint8_t current_time[5]	;
uint8_t writetime_flag = 0	;
volatile uint8_t sost_pkt = 0	;
uint8_t buf_pkt[SIZE_BUF]	;
uint8_t summ, dbg_cntr	;
uint8_t SizePkt, NumMass	;

int main(void)
{	
	//������������� ���������
	Init_peripheral()	;
	USART_Init(2400)	;
	TWI_MasterInit(50)	;
	Init_mbi_ports()	;
	Init_timers()	;	
	OWI_Init(BUS)	;
	
	dev_num = 0	;
	dig_num = 0	;
	ten_seconds_counter = 0	;
	one_second_counter = 0	;
	led_counter = 0	;
	top_led = 0	;
	ten_sec_flag = 0xFF	;
	one_sec_flag = 0xFF	;
	led_flag = 0	;
	out_flag = 0	;
	in_flag = 0	;
	dbg_cntr = 0	;
	
	//sei()	;
	
	//�������� �� ������������� ��������� ������� �� ���������
	Check_time()	;	
	//����� �������� �� ����� 1-Wire
	crcFlag = OWI_SearchDevices(allDevices, MAX_DEVICES, BUS, &sensors_num)	;
	
	//������ � eeprom, � ������ �������������, id ��������� �������� � ���������� ���������� id
	ID_Registration(sensors_num, crcFlag, local_id, allDevices)	;
	
    /* Replace with your application code */
    while (1) 
    {
	    //�������� �������� ������ UART
	    Parse_UART()	;
				
		//�������� ��������� ������
		if (in_flag == 0)
		{
			sost_pkt = 0	;
		}
		
		//������ 10 ������
		if (ten_sec_flag)
		{			
			ten_sec_flag = 0	;
			//���������� ����������� �� ���� ��������
			for (uint8_t i = 0; i < sensors_num; i++)
			{
				crcFlag = DS18B20_ReadTemperature(BUS, allDevices[i].id, (temperature + i))	;
				if (crcFlag == READ_CRC_ERROR)
				{
					temperature[i] = 0x7D0	;
				}
			}
			
			//�������� ���� �������� ��� ������ 16-���������� ���������
			GetLetter(letters, local_id, sensors_num)	;

			//����� ����������� �� ����������
			GetDisplayCode(led_codes, temperature, sensors_num)	;
			
			//���������� �������, �������� ������� � ������ (���� ����������) �� ������� ����������������� ������
			WriteTemperatureTime(local_id, sensors_num, temperature, &writetime_flag)	;
			
			//�������� �������� ���������� ������ �� ������� eeprom
			Check_memory_space(&top_led)	;
		}

		if (one_sec_flag)
		{
			//�������� ������� ������
			temp = PINA	;
			temp &= (1<<VCC_BTN)	;
			if (temp != in_flag)
			{
				in_flag = temp	;
				dbg_cntr = 3	;
			}
			//�������� ������,�� ������� ������� ����������, ������ ��� ����, ����� ������ ������ ���������
			if (dbg_cntr)
			{
				USART_PutChar(0x55)	;
				dbg_cntr--	;
			}
			
			one_sec_flag = 0	;
			//��������� �������
			Brightness_measure(&brightness_l, &brightness_h)	;
			OCR1AL = brightness_l	;
			OCR1AH = brightness_h	;
		}
    }
}

//���������� ���������� �� ���������� ������� 0
ISR(TIMER0_COMP_vect)
{
	Change_digit(sensors_num)	;
}

//���������� ���������� �� ������������ ������� 1
ISR(TIMER1_OVF_vect)
{
	
}

//���������� ���������� �� ���������� ������� 2
ISR(TIMER2_COMP_vect)
{
	MBISignalGenerator()	;
}

void Parse_UART(void)
{
	if (USART_GetRxCount()>0)
	{
		switch (sost_pkt)
		{
			case 0:
			if (USART_GetChar() == 'S')
			{
				sost_pkt = 1;
				summ = 0	;
			}
			break;
			
			case 1:
				SizePkt = buf_pkt[0] = USART_GetChar()	;
				summ += buf_pkt[0]	;
				sost_pkt = 2	;
				if (SizePkt > SIZE_BUF) sost_pkt = 0	;
			break;
			
			case 2:
				NumMass = buf_pkt[1] = USART_GetChar()	;
				summ += buf_pkt[1]	;
				sost_pkt = 3	;
				if (NumMass > 0x10) sost_pkt = 0	;
			break;
			
			default:
				buf_pkt[sost_pkt - 1] = USART_GetChar()	;
				if (sost_pkt < SizePkt)
				{
					summ += buf_pkt[sost_pkt - 1]	;
					sost_pkt++	;
				}
				else
				{
					if (summ == buf_pkt[SizePkt - 1]) RS_Decode((uint8_t *)&buf_pkt[2], NumMass, (SizePkt-3))	;
					sost_pkt = 0	;				
				}
			break;
		}
	}
}

void RS_Decode(uint8_t *data, uint8_t comand, uint8_t SizePkt)
{
	switch (comand)
	{
		case 0:
		break;

		//������� ����� � ��������� �� ����
		case 1:
			SendTime(SizePkt, comand)	;
		break;

		//�������� �����, ���������� �� UART
		case 2:
			WriteTime(SizePkt, data)	;
		break;

		//������� ����� �� ���������� ������ eeprom � ���������� ����� 8 �� ���������� ������,
		//������������ ������� � ��������� � �� ����
		case 3:
			SendEEPROMBytes(SizePkt, data)	;
		break;
		
		//������� ����� �� ������� ������ eeprom � ���������� ����� 8 �� ���������� ������,
		//������������ ������� � ��������� � �� ����
		case 4:
			Send25AA1024Bytes(SizePkt, data)	;
		break;
		
		//������� ������� ����� ������� eeprom (����� ������������ � ������ �����������)
		case 5:
			SendCurrentAddress()	;
		break;
		
		//������� � ��������� �������� ��� ���������� �� ������� ������ ����������
		case 6:
			ReadAllTelemetry()	;
		break;

		//���������� ������� ����� ���������� eeprom (����� ������������ � ������ �����������)
		//������ ������������ ������ ������:
		// |S|...|Pkt_Size|...|Pkt_number|...|Address_0|...___...|Address_3|...|Summ|
		// |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 7 - ����������������� ����� ������;
		// |Summ| - ����������� �����; |Address_3|...___...|Address_0| - ������ ����� ������.
		case 7:
			Write_reserved_string_mega_eeprom(2, data, 4)	;
		break;
		
		//�������� �� ���������� ������ eeprom ��� ������� ��� ������ �� 16-���������� ���������
		case 8:
			WriteLetterCodeToMemory(data, local_id)	;
		break;
		
		default:
		break;
	}
}