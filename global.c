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
#include "global.h"

//настройка (инициализация) периферии
//данная функция настраивает только ту периферию, которая не настраивается в других файлах
void Init_peripheral(void)
{
	SPI_Init()	;
	Timer1_Init()	;
	ADC_Init()	;
	Ports_Init()	;
}

//настройка таймера 1 для управления яркостью индикаторов
void Timer1_Init(void)
{
	uint8_t temp	;
	
	TCCR1A = (1<<COM1A1) | (1<<COM1A0) | (1<<WGM11) | (1<<WGM10)	;		//режим fast pwm, установить лог. 0 на выходе OC1A при TCNT = 0, 
	TCCR1B = (1<<WGM12) | (1<<CS10);										//установить лог. 1 при совпадении с OCR1A, предделитель - 256
	OCR1A = 0x80	;														//яркость при включении средняя
	temp = TIMSK	;
	temp |= (1<<TOIE1)	;
	TIMSK = temp	;
}

//настройка АЦП - измерение напряжения на фоторезисторе для определения яркости внешненго освещения
void ADC_Init(void)
{
	ADMUX = (1<<REFS0) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0)	;			//опорное напряжение - напряжение питания, 7-ой канал АЦП
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0)	;		//предделитель - 128, запуск - вручную
}


void Ports_Init(void)
{
	uint8_t temp	;
	
	temp = OC1A_DDR	;					//включить пин OC1A на выход
	temp |= (1<<OC1A_BIT)	;
	OC1A_DDR = temp	;

	//светодиод
	temp = DDRC	;
	temp |= (1<<LED)	;
	DDRC = temp;
	temp = PORTC	;
	temp |= (0<<LED)	;
	PORTC = temp	;
	//кнопка
	temp = DDRA	;
	temp &= ~(1<<VCC_BTN)	;
	DDRA = temp	;
	temp = PORTA	;
	temp &= ~(1<<VCC_BTN)	;
	PORTA = temp	;
}

/*****************************************************************************
*   Function name :   DS18B20_ReadTemperature
*   Returns :       коды - READ_CRC_ERROR, если считанные данные не прошли проверку
*                          READ_SUCCESSFUL, если данные прошли проверку    
*   Parameters :    bus - вывод микроконтроллера, который выполняет роль 1WIRE шины
*                   *id - имя массива из 8-ми элементов, в котором хранится
*                         адрес датчика DS18B20
*                   *temperature - указатель на шестнадцати разрядную переменную
*                                в которой будет сохранено считанного зн. температуры
*   Purpose :      Адресует датчик DS18B20, дает команду на преобразование температуры
*                  ждет, считывает его память - scratchpad, проверяет CRC,
*                  сохраняет значение температуры в переменной, возвращает код ошибки             
*****************************************************************************/
unsigned char DS18B20_ReadTemperature(unsigned char bus, unsigned char * id, unsigned int* temperature)
{
    unsigned char scratchpad[9];
    unsigned char i;
  
    /*подаем сигнал сброса
    команду для адресации 1-Wire устройства на шине
    подаем команду - запук преобразования */
    OWI_DetectPresence(bus);
    OWI_MatchRom(id, bus);
    OWI_SendByte(DS18B20_CONVERT_T ,bus);

    /*ждем, когда датчик завершит преобразование*/ 
    while (!OWI_ReadBit(bus));

    /*подаем сигнал сброса
    команду для адресации 1-Wire устройства на шине
    команду - чтение внутренней памяти
    затем считываем внутреннюю память датчика в массив
    */
    OWI_DetectPresence(bus);
    OWI_MatchRom(id, bus);
    OWI_SendByte(DS18B20_READ_SCRATCHPAD, bus);
    for (i = 0; i<=8; i++){
      scratchpad[i] = OWI_ReceiveByte(bus);
    }
    
    if(OWI_CheckScratchPadCRC(scratchpad) != OWI_CRC_OK){
      return READ_CRC_ERROR;
    }
    
    *temperature = (unsigned int)scratchpad[0];
    *temperature |= ((unsigned int)scratchpad[1] << 8);
    
    return READ_SUCCESSFUL;
}

//запись в eeprom, в случае необходимости, id найденных датчиков и присвоение локального id
void ID_Registration(unsigned char sensors_num, unsigned char crcFlag, uint8_t *local_id, OWI_device *allDevices)
{
	if ((sensors_num != 0) && (crcFlag != SEARCH_CRC_ERROR))
	{
		uint8_t status, infobyte, number	;
		
		//если найдены датчики, то сначала проверяется производилась ли уже запись в eeprom.
		//Если уже происходила запись в eeprom, то по адресу 1 будет значение равное 0x77
		//Важно: запись и чтение в eeprom производится с двойным резервированием (см. описание функций)
		status = Read_info_byte_mega_eeprom(1, &infobyte)	;
		if (status)
		{
			if (infobyte == 0x77)
			{
				//если в памяти уже записаны зав. номера датчиков
				Read_info_byte_mega_eeprom(0, &number)	;
				if (status)
				{
					for (uint8_t i = 0; i<sensors_num; i++)									//сравнение зав. номеров найденных датчиков
					{
						local_id[i] = 0	;
						for(uint8_t j = 1; j <= number; j++)								//с зав. номерами уже записанных и присвоение локального номера каждому найденому датчику
						{
							uint16_t address	;
							unsigned char eep[8]	;
							int cmp	;
							
							address = j*8	;
							
							status  = Read_reserved_string_mega_eeprom(address, eep, 8)	;
							cmp = memcmp(allDevices[i].id, eep, 8)	;
							
							if ((status != 0) && (cmp == 0))
							{
								local_id[i] = j	;
							}
						}
						if (local_id[i] == 0)
						{
							uint16_t address	;
							
							number++	;
							Write_info_byte_mega_eeprom(0, number)	;
							address = number*8	;
							Write_reserved_string_mega_eeprom(address, allDevices[i].id, 8)	;
							local_id[i] = number	;
						}
					}
				}
				else
				{
					//если не удалось правильно считать байт о количестве записанных данных
					//обнулить байт количества и байт-признак записи
					infobyte = 0	;
					
					Write_info_byte_mega_eeprom(0, infobyte)	;
					Write_info_byte_mega_eeprom(1, infobyte)	;
				}
			}
			else
			{
				//если это первая запись в память, то просто записать все заводские номера найденных датчиков в память
				infobyte = 0x77	;				
				unsigned char eep[4] = {0, 0, 0, 0}	;
				
				Write_info_byte_mega_eeprom(0, sensors_num)	;
				Write_info_byte_mega_eeprom(1, infobyte)	;
				Write_reserved_string_mega_eeprom(2, eep, 4)	;				//адрес во внешней памяти, с которого начнется запись 
				
				for (uint8_t i = 1; i <= sensors_num; i++)
				{
					uint16_t address	;
					
					address = i*8	;
					Write_reserved_string_mega_eeprom(address, allDevices[(i-1)].id, 8)	;
					local_id[(i-1)] = i	;
				}
			}
		}
		else
		{
			
			//если при чтении произошла ошибка, то просто записать все заводские номера найденных датчиков в память
			infobyte = 0x77	;
			Write_info_byte_mega_eeprom(0, sensors_num)	;
			Write_info_byte_mega_eeprom(1, infobyte)	;
			for (uint8_t i = 1; i <= sensors_num; i++)
			{
				uint16_t address	;
				
				address = i*8	;
				Write_reserved_string_mega_eeprom(address, allDevices[(i-1)].id, 8)	;
			}
		}
	}
}

//измерение яркости с помощью фоторезистора
//измеряется напряжение на нём
void Brightness_measure(uint8_t *brightness_l, uint8_t *brightness_h)
{
	unsigned char templ, temph	;
	uint32_t rtd_vlt, voltage	;
	
	templ = ADCSRA	;
	templ |= (1<<ADSC)	;
	ADCSRA = templ	;
	while ((ADCSRA & (1<<ADSC)) != 0)	;
	templ = ADCL	;
	temph = ADCH	;
	voltage = temph	;
	voltage = voltage << 8	;
	voltage |= templ	;
	
	if (voltage > 280)
	{
		if (voltage > 980)
		{
			voltage = 980	;
		} 
		voltage = voltage - 280	;
		rtd_vlt = voltage*1462	;
		voltage = rtd_vlt/1000	;
		voltage--	;	
	} 
	else
	{
		voltage = 0	;
	}
	templ = voltage	;
	temph = voltage >> 8	;
	*brightness_l = templ	;
	*brightness_h = temph	;
}

//преобразование 4-х однобайтовых переменных в одну 4-х байтную 
 void Convert4_to_1(uint8_t *data, uint32_t *dword)
 {
	 uint8_t temp_3, temp_2, temp_1, temp_0	;
	 uint32_t temp	;

	 temp_0 = *data++	;
	 temp_1 = *data++	;
	 temp_2 = *data++	;
	 temp_3 = *data++	;
	 
	 temp = temp_3	;
	 temp = (temp<<8) | temp_2	;
	 temp = (temp<<8) | temp_1	;
	 temp = (temp<<8) | temp_0	; 
	 
	 *dword = temp	;
 }

 //преобразование одной 4-х байтной переменной в 4 однобайтных
 void Convert1_to_4(uint8_t *data, uint32_t *dword)
 {
	 uint32_t temp	;

	temp = *dword	;
	 *data++ = temp	;
	 *data++ = (temp<<8)	;
	 *data++ = (temp<<16)	;
	 *data++ = (temp<<24)	;
 }
 
 //проверка количества оставшейся памяти во внешней eeprom
 //если свободной памяти остаётся меньше четверти от общего количества, 
 //то происходит моргание светодиода, чем меньше памяти, тем чаще моргание
 void Check_memory_space(unsigned int *top_led)
 {
	 uint8_t data[4]	;
	 uint32_t address	;
	 uint32_t period	;
	 
	 Read_reserved_string_mega_eeprom(2, data, 4)	;
	 Convert4_to_1(data, &address)	;

	 if (address > 0x3000)
	 {
		 address = address - 0x3000	;
		 period = address/0x200	;
		 period = 8 - period	;
		 period = period*TIM0_FREQENCY	;
		 *top_led = period	;
	 } 
	 else
	 {
		 *top_led = 0	;
	 }
 }
 
 //получить код символа для отображения на 16-сегментном индикаторе
 //код считывают из памяти eeprom 
 void GetLetter(unsigned int * letter, uint8_t * local_id, unsigned char sensors_num)
 {
	 uint8_t status	;
	 uint16_t address	;
	 uint8_t data[2]	;	 
	 
	 for (uint8_t i = 0; i < sensors_num; i++)
	 {
		 address = 136 + (local_id[i] - 1)*2	;
		 status = Read_reserved_string_mega_eeprom(address, data, 2)	;
		 if (status)
		 {
			 letter[i] = data[1]	;
			 letter[i] = (letter[i]<<8) | data[0]	;
		 } 
		 else
		 {
			 letter[i] = 0	;
		 }
	 }
 }
 
 //считывание времени и запись во внешнюю память значения температуры и времени для каждого датчика (каждые 10 минут)
 void WriteTemperatureTime(uint8_t *local_id, uint8_t sensors_num, unsigned int *temperature, uint8_t *flag)
 {
	uint8_t buf[8], minutes	;
			
	//Считать с датчика время.
	//сначала установить адрес регистра, с которого начнётся считывание данных
	buf[0] = (DS1307_ADR<<1)|0	;						//адрес устройства
	buf[1] = 0	;										//адрес, с которого будем считывать
	TWI_SendData(buf, 2)	;
	//считывание
	buf[0] = (DS1307_ADR<<1)|1	;
	TWI_SendData(buf, 8)	;
	TWI_GetData(buf, 8)	;					//теперь время и дата находятся в буфере buf
	minutes = buf[2]%(0x10)	;				//в buf[2] находится значение минут, если оно кратно 10, то результат деления будет равен 0
	
	if (minutes == 0)
	{
		if (*flag == 0)
		{
			*flag = 0xFF	;
			for(uint8_t i = 0; i < sensors_num; i++)
			{
				uint8_t status	;
				unsigned char eep[4] = {0, 0, 0, 0}	;
				
				status  = Read_reserved_string_mega_eeprom(2, eep, 4)	;			//считать адрес из внутренней eeprom, по которому будут записываться данные
				if (status)
				{
					uint32_t address	;
					uint8_t wrbuf[8]	;
					uint8_t templ, temph	;
					unsigned int temp16	; 
					
					address = eep[3]	;											//положить адрес из 32-битной переменной в 4 8-битных 
					address = (address<<8) | eep[2]	;
					address = (address<<8) | eep[1]	;
					address = (address<<8) | eep[0]	;
					
					//положить данные для записи в память в массив из 8 байт
					wrbuf[0] = *(local_id + i)	;			//локальный номер датчика
					wrbuf[1] = buf[7]	;						//год
					wrbuf[2] = buf[6]	;					//месяц
					wrbuf[3] = buf[5]	;					//день
					wrbuf[4] = buf[3]	;					//часы
					wrbuf[5] = buf[2]	;					//минуты
					
					temp16 = *(temperature + i)	;			//температура 
					templ = temp16	;
					temp16 = temp16>>8	;
					temph = temp16	;
					wrbuf[6] = temph	;
					wrbuf[7] = templ	;
					
					Write_string_ext_eeprom(address, wrbuf, 8)	;					//записать данные во внешнюю память
					address = address + 8	;										//увеличить адрес на 8
					
					eep[0] = address	;
					eep[1] = address>>8	;
					eep[2] = address>>16	;
					eep[3] = address>>24	;
					Write_reserved_string_mega_eeprom(2, eep, 4)	;
				} 
				else
				{
					Write_reserved_string_mega_eeprom(2, eep, 4)	;		//если при чтении произошла ошибка - обнулить адрес внешней 
				}
			}
		}
	} 
	else
	{
		*flag = 0	;
	}
 }
 
 //отправка времени, считанного с датчика ds1307 по УАРТ
 //сформировать посылку и отправить её по уарт
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...___...|Dn|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 1 - идентификационный номер пакета;
 // |D0|...|D1|...___...|Dn| - пустые данные, где n - количество информационных байтов.
 // формат отправляемого пакета данных аналогичен принятому
 void SendTime(uint8_t SizePkt, uint8_t command)
 {
	 uint8_t buf[SizePkt+2]	;
	 uint8_t output_buf[SizePkt+4]	;
	 uint8_t summa = 0, temp	;
	 
	 temp = SizePkt	;
	 //Считать с датчика время
	 buf[0] = (DS1307_ADR<<1)|0	;						//адрес устройства
	 buf[1] = 0	;										//адрес, с которого будем считывать
	 TWI_SendData(buf, 2)	;
	 //считывание
	 buf[0] = (DS1307_ADR<<1)|1	;
	 TWI_SendData(buf, temp+2)	;
	 TWI_GetData(buf, temp+1)	;								//теперь время и дата находятся в буфере buf начиная с индекса 1
	 //формирование пакета для отправки
	 output_buf[0] = 0x53	;
	 output_buf[1] = SizePkt + 3	;
	 summa = output_buf[1]	;
	 output_buf[2] = command	;
	 summa += output_buf[2]	;
	 
	 for(uint8_t i = 3; i < (SizePkt + 3); i++)
	 {
		 output_buf[i] = buf[i-2]	;
		 summa += output_buf[i]	;
	 }
	 output_buf[SizePkt + 3] = summa	;	
	 USART_SendArray(output_buf, SizePkt + 4)	; 
 }
 
 //запись времени, полученного по УАРТ
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...___...|Dn|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| - идентификационный номер пакета;
 // |D0|...|D1|...___...|Dn| - время для записи, начиная с секунд.
 void WriteTime(uint8_t SizePkt, uint8_t *data)
 {
	 uint8_t buf[SizePkt + 2]	;
	 	 
	 buf[0] = (DS1307_ADR << 1) | 0	;
	 buf[1] = 0	;
	 for (uint8_t i = 2; i < (SizePkt + 2); i++)
	 {
		 buf[i] = *data++	;
	 }
	 
	 TWI_SendData(buf, (SizePkt + 2))	;
 }
 
 //считать байты из внутренней памяти eeprom в количестве менее 8 по указанному адресу,
 //сформировать посылку и отправить её по уарт
 //формат принимаемого пакета данных: 
 // |S|...|Pkt_Size|...|Pkt_number|...|Read_Size|...|Address_H|...|Address_L|...|D0|...|D1|...___...|Dn|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| - идентификационный номер пакета;
 // |Read_Size| - количество байт, считываемых из памяти; |Address_H|,|Address_L| - старший и младший байт адреса, с которого начинается считывание данных
 // |D0|,|D1|,...,|Dn| - нулевые данные (количество байт д.б. равно Read_Size); |Summ| - контрольная сумма.
 //формат отправляемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...___...|Dn|...|Summ|
 void SendEEPROMBytes(uint8_t SizePkt, uint8_t *data)
 {
	 uint8_t output_buf[SizePkt+1]	;
	 uint8_t buf[SizePkt - 3]	;
	 uint8_t summa = 0	;
	 uint16_t eep_address	;
	 uint8_t eep_address_high, eep_address_low	;
	 uint8_t eep_pkt_size	;
	 
	 eep_pkt_size = *data++	;
	 eep_address_high = *data++	;
	 eep_address_low = *data++	;
	 eep_address = eep_address_low | (eep_address_high<<8)	;
	 
	 Read_string_mega_eeprom(eep_address, buf, eep_pkt_size)	;
	 //ответный пакет для отправки по UART
	 output_buf[0] = 0x53	;
	 output_buf[1] = SizePkt	;
	 summa = output_buf[1]	;
	 output_buf[2] = 0x3	;
	 summa += output_buf[2]	;
	 
	 for (uint8_t i = 3; i < SizePkt; i++)
	 {
		output_buf[i] = buf[i-3]	; 
		summa += output_buf[i]	;
	 }	
	 output_buf[SizePkt] = summa	;
	 USART_SendArray(output_buf, (SizePkt + 1))	;
 }
 
  //считать байты из внешней памяти eeprom (25AA1024) в количестве менее 8 байт по указанному адресу,
  //сформировать посылку и отправить её по уарт
  //формат принимаемого пакета данных:
  // |S|...|Pkt_Size|...|Pkt_number|...|Read_Size|...|Address_3|...___...|Address_0|...|D0|...|D1|...___...|Dn|...|Summ|
  // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 4 - идентификационный номер пакета;
  // |Read_Size| - количество байт, считываемых из памяти; |Address_3|,...,|Address_0| - четыре байта адреса, с которого начинается считывание данных
  // |D0|,|D1|,...,|Dn| - нулевые данные (количество байт д.б. равно Read_Size); |Summ| - контрольная сумма.
  //формат отправляемого пакета данных:
  // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...___...|Dn|...|Summ|
 void Send25AA1024Bytes(uint8_t SizePkt, uint8_t *data)
 {	 
	 uint8_t output_buf[SizePkt - 1]	;
	 uint8_t buf[SizePkt - 1]	;
	 uint8_t summa = 0	;
	 uint32_t eep_address	;
	 uint8_t temp_3, temp_2, temp_1, temp_0	;
	 uint8_t eep_pkt_size	;
	 
	 eep_pkt_size = *data++	;
	 temp_3 = *data++	;
	 temp_2 = *data++	;
	 temp_1 = *data++	;
	 temp_0 = *data++	;
	 eep_address = temp_3 ;
	 eep_address = (eep_address << 8) | temp_2	;
	 eep_address = (eep_address << 8) | temp_1	;
	 eep_address = (eep_address << 8) | temp_0	;
	 
	 Read_string_ext_eeprom(eep_address, buf, eep_pkt_size)	;
	 //ответный пакет для отправки по UART
	 output_buf[0] = 0x53	;
	 output_buf[1] = SizePkt - 2	;
	 summa = output_buf[1]	;
	 output_buf[2] = 0x4	;
	 summa += output_buf[2]	;
	 
	 for (uint8_t i = 3; i < (SizePkt - 2); i++)
	 {
		 output_buf[i] = buf[i+1]	;
		 summa += output_buf[i]	;
	 }
	 output_buf[SizePkt-2] = summa	;
	 USART_SendArray(output_buf, (SizePkt - 1))	;
 }
 
 //считать из внутренней памяти eeprom 4 байта текущего адреса во внешней памяти eeprom (25AA1024) 
 //сформировать посылку и отправить её по уарт
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 5 - идентификационный номер пакета;
 // |Summ| - контрольная сумма.
 // формат отправляемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Address_3|...___...|Address_0|...|Summ|
 // |Address_3|...___...|Address_0| - текущий адрес побайтно
 void SendCurrentAddress(void)
 {
	 uint8_t status	;
	 uint8_t eep[4]	;
	 uint8_t output_buf[8]	;	 
	 
	 status  = Read_reserved_string_mega_eeprom(2, eep, 4)	;			//считать адрес из внутренней eeprom
	 if (status)
	 {
		 uint8_t summa	;
		 
		 output_buf[0] = 0x53	;
		 output_buf[1] = 7	;
		 summa = 7	;
		 output_buf[2] = 5	;
		 summa += output_buf[2]	;
		 
		 for(uint8_t i = 0; i < 4; i++)
		 {
			 output_buf[i + 3] = eep[3-i]	;
			 summa += output_buf[i + 3]	;
		 } 
		 output_buf[7] = summa	;
		 USART_SendArray(output_buf, 8)	;
	 }
 }
 
 //считать из внешней памяти eeprom все данные телеметрии
 //отправить всё по уарт пакетами, кол-во пакетов = адрес последнего записанного байта разделить на 8;
 //формат принимаемого пакета данных(RECEIVE):
 // |S|...|Pkt_Size|...|Pkt_number|...|Type|...|Summ|
 // |S| = 0x53 - стартовый байт; 
 // |Pkt_Size| - общее количество байт в пакете; 
 // |Pkt_number| = 6 - идентификационный номер пакета;
 // |Type| - тип пакета, ожидаемого приёмником;
 // - 0x01 - начать отправку записанных в памяти данных передачей служебных данных;
 // - 0х02 - приёмник подтверждает приём служебных данных, необходимо отправить первые 8 байт данных из памяти с адреса 0;
 // - 0х03 - передать следующие 8 байт данных из памяти (увеличить счетчик адреса на 8 и передать следующие 8 байт данных из памяти) ;
 // - 0х04 - повторить передачу 8 байт данных из памяти (счетчик адреса не изменять);
 // - 0х05 - приёмник принял все пакеты, которые планировалось принять (количество передаваемых пакетов содержалось в служебных данных);
 // - 0х06 - прервать передачу данных и обнулить счётик адреса;
 // |Summ| - контрольная сумма.
 // формат отправляемого пакета данных(SEND):
 // |S|...|Pkt_Size|...|Pkt_number|...|Type|...|Depend on type|...|Summ| 
 // |S| = 0x53 - стартовый байт;
 // |Pkt_Size| - общее количество байт в пакете;
 // |Pkt_number| = 6 - идентификационный номер пакета;
 // |Type| - тип отправляемого пакета;
 // - 0x01 - пакет содержит служебные данные;
 //  |Depend on type| = |Address_3|...___...|Address_0|...|Sensors_number|...|Local_sensor0_id|...|Sensor0_Letter_1|...
 // ...|Sensor0_Letter_0|...|Sensor0_id_8|...___...|Sensor0_id_0|...______...|Local_sensorN_id|...|SensorN_Letter_1|...
 // ...|SensorN_Letter_0|...|SensorN_id_8|...___...|SensorN_id_0|
 //   -- |Address_3|...___...|Address_0| - текущий адрес внешней памяти, равен количеству байт, записанных в память и, соответственно,
 //   общему количеству передаваемых данных;
 //	  -- |Sensors_number| - количество записанных во внутреннюю память датчиков;
 //	  -- |Local_sensor0_id| - локальный id самого первого записанного в память датчика(с нулевым индексом);
 //   -- |Sensor0_Letter_1|...|Sensor0_Letter_0| - два байта цифрового кода буквенного символа самого первого записанного в память датчика;
 //   -- |Sensor0_id_8|...___...|Sensor0_id_0| - 8 байт с id самого первого записанного в память датчика(с нулевым индексом);
 //	  -- |Sensors_number| - количество записанных во внутреннюю память датчиков;
 //	  -- |Local_sensorN_id| - локальный id N-го записанного в память датчика(N = Sensors_number - 1);
 //   -- |SensorN_Letter_1|...|SensorN_Letter_0| - два байта цифрового кода буквенного символа N-го записанного в память датчика;
 //   -- |SensorN_id_8|...___...|SensorN_id_0| - 8 байт с id N-го записанного в память датчика;
 // - 0х02 - передача 8 байт данных;
 //   |Depend on type| = |D0|...|D1|...___...|Dn|
 //   -- |D0|,|D1|,...,|Dn| - 8 байт данных, где D0 считано с адреса, кратного 8 (или с адреса 0), а Dn - с адреса, кратного n;
 // - 0х03 - отправка дополнительных данных: 
 //			приёмник получил все данные, что планировал, но у передатчика остались ещё данные для отправки и он их отправляет.
 //          Такая ситуация может произойти, если в процессе передачи данных произошла очередная запись времени и температуры в память
 //			 и текущий адрес внешней памяти EEPROM отличается от счетчика адреса передаваемых данных;
 //   |Depend on type| = |D0|...|D1|...___...|Dn|
 //   -- |D0|,|D1|,...,|Dn| - 8 байт данных, где D0 считано с адреса, кратного 8 (или с адреса 0), а Dn - с адреса, кратного n;
 // - 0х04 - приёмник передал все данные;
 //   |Depend on type| = Nothing;
 // |Summ| - контрольная сумма.
 void ReadAllTelemetry(void)
 {
	 uint8_t status	;
	 uint8_t eep[4]	;
	 uint32_t pkt_nmbr = 0	;											//количество пакетов
	 
	 status  = Read_reserved_string_mega_eeprom(2, eep, 4)	;			//считать адрес из внутренней eeprom
	 if(status)
	 {
		uint32_t address	;
		
		address = eep[3]	;											//перевести 4 байта адреса в 32-битную переменную
		address = (address<<8) | eep[2]	;
		address = (address<<8) | eep[1]	;
		address = (address<<8) | eep[0]	;
		address = address & (0xFFFFFF)	;
		
		pkt_nmbr = address/8	;										//вычисление количества пакетов
	 }
	 
	 if (pkt_nmbr)
	 {
		 for (uint32_t i = 0; i < pkt_nmbr; i++)
		 {
				uint8_t buf[12], output_buf[12]	;
				uint8_t summa	;
				
				while(USART_GetTxCount());			 	 
			 	Read_string_ext_eeprom(i*8, buf, 8)	;
			 	//ответный пакет для отправки по UART
			 	output_buf[0] = 0x53	;
			 	output_buf[1] = 0xB	;
			 	summa = output_buf[1]	;
			 	output_buf[2] = 0x6	;
			 	summa += output_buf[2]	;
			 	 
			 	for (uint8_t j = 3; j < 11; j++)
			 	{
					output_buf[j] = buf[j + 1]	;
				 	summa += output_buf[j]	;
			 	}
			 	output_buf[11] = summa	;
			 	USART_SendArray(output_buf, 12)	;
		 }
	 }
 }
 
 //записать во внутреннюю память eeprom код (2 байта) символа для вывода на 16-сегментный индикатор
 //адрес, по которому  записывается код вычисляется по номеру отображаемого на индикаторах датчика
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Number|...|Letter1|...|Letter0|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 8 - идентификационный номер пакета;
 // |Summ| - контрольная сумма; |Address_3|...___...|Address_0| - четыре байта адреса.
 void WriteLetterCodeToMemory(uint8_t *data, uint8_t * local_id)
 {
	 uint8_t number	;
	 uint16_t address	;
	 uint8_t letter[2]	;
	 
	 number = *data++	;
	 letter[1] = *data++	;
	 letter[0] = *data++	;
	 
	 address = 136 + (local_id[number] - 1)*2	;
	 Write_reserved_string_mega_eeprom(address, letter, 2)	;
 }
 
 //отправить текущую температуру с информацией о датчике (идентификационный номер, код символа, связанного с ним)
 //для всех датчиков
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 9 - идентификационный номер пакета;
 // |D0|, |D1| - пустые данные
 // |Summ| - контрольная сумма.
 //формат отправляемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Sensors_number|...|Sensor0_id_8|...___...|Sensor0_id_0|...|Sensor0_Letter_1|...
 // ...|Sensor0_Letter_0|...|Sensor0_Temperature_1|...|Sensor0_Temperature0|...___...|SensorN_id_8|...___...|SensorN_id_0|...|SensorN_Letter_1|...
 // ...|SensorN_Letter_0|...|SensorN_Temperature_1|...|SensorN_Temperature0|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 9 - идентификационный номер пакета;
 // |Sensors_number| - общее количество датчиков на линии
 // |SensorN_id_N| - N-ый байт идентификационного номера N-го датчика,
 // |SensorN_Letter_1|...|SensorN_Letter_0| - код символа, ассоциированного с датчиком N,
 // |SensorN_Temperature_1|...|SensorN_Temperature0| - два байта с текущей температурой, измеренных N-ым датчиком
 // |Summ| - контрольная сумма пакета.
 void SendSensorsTemperature(unsigned char sensors_num, unsigned int * temperature, uint8_t * local_id, OWI_device * allDevices)
 {
	 //объявление буффера, куда складываются данные для отправки по УАРТ
	 //размер буффера будет зависеть от количества датчиков на линии
	 uint8_t buffer_size = 5 + sensors_num*(SENSOR_INFO_SIZE)	;
	 uint8_t output_buf[buffer_size]	;
	 uint16_t address, temp	;
	 uint8_t letter[2]	;
	 uint8_t templ	;
	 uint8_t temph	;
	 uint8_t summa = 0	;
	 
	 output_buf[0] = 0x53	;
	 output_buf[1] = buffer_size - 1	;
	 output_buf[2] = 9	;
	 output_buf[3] = sensors_num	;
	 
	 //заполнение пакета данными датчиков - id, код символа и текущая темппература, измеренная датчиком
	 for(uint8_t i = 0; i < sensors_num; i++) {
		 uint8_t index = 4 + SENSOR_INFO_SIZE*i	;
		 
		 address = 136 + (local_id[i] - 1)*2	;
		 Read_reserved_string_mega_eeprom(address, letter, 2)	;
		 
		 temp = temperature[i]	;
		 templ = temp	;
		 temph = temp>>8	;
		 
		 ////id датчика
		 for(uint8_t j = 0; j < SENSOR_ID_SIZE; j++) {
			output_buf[index] = allDevices[i].id[j]	;
			index++	;
		 }
		 
		 //код символа		 
		 output_buf[index++] = letter[1]	;
		 output_buf[index++] = letter[0]	;
		 
		 //температура		 
		 output_buf[index++] = temph	;
		 output_buf[index++] = templ	;		   
	 }	 
	 //
	 ////вычисление контрольной суммы
	 for(uint8_t i = 1; i < (buffer_size - 1); i++) {
		summa = summa + output_buf[i]	;
	 }
	 output_buf[buffer_size - 1] = summa	;
	 
	 USART_SendArray(output_buf, buffer_size)	;
 }
 
 
 //отправить текущее значение яркости 
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 10 - идентификационный номер пакета;
 // |D0|, |D1| - пустые данные
 // |Summ| - контрольная сумма.
 //формат отправляемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Brightness_H|...|Brightness_L|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 10 - идентификационный номер пакета;
 // Brightness_H - старший байт значения яркости, Brightness_L - младший байт значения яркости.
 void SendBrightnessValue(uint8_t brightness_l, uint8_t brightness_h) {
	 
	 uint8_t output_buf[6]	;
	 uint8_t summa = 0	;
	 
	 output_buf[0] = 0x53	;
	 output_buf[1] = 0x5	;
	 summa = 0x5	;
	 output_buf[2] = 0xA	;
	 summa += output_buf[2]	;
	 output_buf[3] = brightness_h	;
	 summa += output_buf[3]	;
	 output_buf[4] = brightness_l	;
	 summa += output_buf[4]	;
	 output_buf[5] = summa	;
	 
	 USART_SendArray(output_buf, 6)	;
 }