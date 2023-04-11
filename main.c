/*
 * Thermometer_3.0.c
 *
 Описание...............
 Основная программа управления термометром 3-ей версии.
 Данная программа выполняет следующие функции:
 - выполняет поиск датчиков температуры на линии (протокол 1-Wire);
 - считывает температуру с найденных датчиков;
 - поочерёдно отображает полученную температуру на светодиодных многосегментных индикаторах;
 - считывает время с датчика DS1307 (протокол TWI);
 - записывает каждые 10 минут значения температур в микросхему памяти 25АА1024 (протокол SPI);
 - определяет уровень освещённости с помощью фоторезистора и регулирует яркость свечения светодиодных индикаторов;
 - подаёт напряжение на светодиод для сигнализации включения bluetooth-модуля и предупреждение о малом количестве оставшейся памяти
 в микросхеме 25АА1024;
 - обменивается информацией и получает команды посредством протокола UART, в том числе передаёт данные из микросхемы памяти 25АА1024;
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
	//инициализация периферии
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
	
	//проверка на необходимость установки времени по умолчанию
	Check_time()	;	
	//Поиск датчиков на линии 1-Wire
	crcFlag = OWI_SearchDevices(allDevices, MAX_DEVICES, BUS, &sensors_num)	;
	
	//запись в eeprom, в случае необходимости, id найденных датчиков и присвоение локального id
	ID_Registration(sensors_num, crcFlag, local_id, allDevices)	;
	
    /* Replace with your application code */
    while (1) 
    {
	    //проверка приёмного буфера UART
	    Parse_UART()	;
				
		//обнулить состояние пакета
		if (in_flag == 0)
		{
			sost_pkt = 0	;
		}
		
		//каждые 10 секунд
		if (ten_sec_flag)
		{			
			ten_sec_flag = 0	;
			//считывание температуры со всех датчиков
			for (uint8_t i = 0; i < sensors_num; i++)
			{
				crcFlag = DS18B20_ReadTemperature(BUS, allDevices[i].id, (temperature + i))	;
				if (crcFlag == READ_CRC_ERROR)
				{
					temperature[i] = 0x7D0	;
				}
			}
			
			//получить коды символов для вывода 16-сегментный индикатор
			GetLetter(letters, local_id, sensors_num)	;

			//вывод температуры на индикаторы
			GetDisplayCode(led_codes, temperature, sensors_num)	;
			
			//считывание времени, проверка времени и запись (если необходимо) во внешнюю энергонезависимую память
			WriteTemperatureTime(local_id, sensors_num, temperature, &writetime_flag)	;
			
			//проверка занятого количества памяти во внешней eeprom
			Check_memory_space(&top_led)	;
		}

		if (one_sec_flag)
		{
			//проверка нажатия кнопки
			temp = PINA	;
			temp &= (1<<VCC_BTN)	;
			if (temp != in_flag)
			{
				in_flag = temp	;
				dbg_cntr = 3	;
			}
			//отправка данных,не несущих никакой информации, только для того, чтобы блютуз модуль заработал
			if (dbg_cntr)
			{
				USART_PutChar(0x55)	;
				dbg_cntr--	;
			}
			
			one_sec_flag = 0	;
			//настройка яркости
			Brightness_measure(&brightness_l, &brightness_h)	;
			OCR1AL = brightness_l	;
			OCR1AH = brightness_h	;
		}
    }
}

//обработчик прерывания по совпадению таймера 0
ISR(TIMER0_COMP_vect)
{
	Change_digit(sensors_num)	;
}

//обработчик прерывания по переполнению таймера 1
ISR(TIMER1_OVF_vect)
{
	
}

//обработчик прерывания по совпадению таймера 2
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

		//считать время и отправить по УАРТ
		case 1:
			SendTime(SizePkt, comand)	;
		break;

		//записать время, полученное по UART
		case 2:
			WriteTime(SizePkt, data)	;
		break;

		//считать байты из внутренней памяти eeprom в количестве менее 8 по указанному адресу,
		//сформировать посылку и отправить её по уарт
		case 3:
			SendEEPROMBytes(SizePkt, data)	;
		break;
		
		//считать байты из внешней памяти eeprom в количестве менее 8 по указанному адресу,
		//сформировать посылку и отправить её по уарт
		case 4:
			Send25AA1024Bytes(SizePkt, data)	;
		break;
		
		//считать текущий адрес внешней eeprom (адрес записывается в память контроллера)
		case 5:
			SendCurrentAddress()	;
		break;
		
		//считать и отправить пакетами всю записанную во внешней памяти телеметрию
		case 6:
			ReadAllTelemetry()	;
		break;

		//установить текущий адрес микросхемы eeprom (адрес записывается в память контроллера)
		//формат принимаемого пакета данных:
		// |S|...|Pkt_Size|...|Pkt_number|...|Address_0|...___...|Address_3|...|Summ|
		// |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 7 - идентификационный номер пакета;
		// |Summ| - контрольная сумма; |Address_3|...___...|Address_0| - четыре байта адреса.
		case 7:
			Write_reserved_string_mega_eeprom(2, data, 4)	;
		break;
		
		//записать во внутреннюю память eeprom код символа для вывода на 16-сегментный индикатор
		case 8:
			WriteLetterCodeToMemory(data, local_id)	;
		break;
		
		default:
		break;
	}
}