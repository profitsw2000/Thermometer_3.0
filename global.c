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

uint32_t send_address = 0	;

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
 //если свободной памяти остаётся меньше половины от общего количества, 
 //то происходит моргание светодиода, чем меньше памяти, тем чаще моргание.
 //Логика определения длительности свечения диода и периода моргания следующий:
 //если текущий адрес(а значит и заполнение памяти) меньше значения MEMORY_INDICATION_TRESHOLD,
 //то светодиод вообще не загорается. Если текущий адрес превысил значение MEMORY_INDICATION_TRESHOLD,
 //то светодиод загорается на время TIM0_FREQENCY/LED_LIGHTNING_COEF (в количествах срабатывания таймера 0),
 //т.е. на 1/LED_LIGHTNING_COEF секунды. Время же паузы зависит от заполнения оставшейся памяти и определяется
 //следующим образом: оставшаяся память делится на MEMORY_INDICATION_FREQ_NUMBER секторов. 
 //Вычисляется количество незаполненных секторов (и +1) и умножается на TIM0_FREQENCY/LED_LIGHTNING_COEF.
 //Получится время паузы между подачей питания на светодиод в количествах срабатывания таймера 0.
 void Check_memory_space(uint16_t *led_pulse_duration, uint16_t *led_pulse_period)
 {
	 uint8_t data[4]	;
	 uint32_t address	;
	 uint32_t period, duration	;	 
	 
	 Read_reserved_string_mega_eeprom(2, data, 4)	;
	 Convert4_to_1(data, &address)	;
	 	 
	 if (address < MEMORY_INDICATION_TRESHOLD) {
		 duration = 0	;
		 period = MEMORY_INDICATION_FREQ_NUMBER*(TIM0_FREQENCY/LED_LIGHTNING_COEF)	;
	 }
	 else {
		 duration = TIM0_FREQENCY/LED_LIGHTNING_COEF	;
		 period = (MEMORY_INDICATION_FREQ_NUMBER + 1 - (address - MEMORY_INDICATION_TRESHOLD)/MEMORY_PART_SIZE)*(TIM0_FREQENCY/LED_LIGHTNING_COEF)	;
	 }
	 
	 *led_pulse_period = period	;
	 *led_pulse_duration = duration	;

	 //if (address > 0x3000)
	 //{
		 //address = address - 0x3000	;
		 //period = address/0x200	;
		 //period = 8 - period	;
		 //period = period*TIM0_FREQENCY	;
		 //*top_led = period	;
	 //} 
	 //else
	 //{
		 //*top_led = 0	;
	 //}
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
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 6 - идентификационный номер пакета;
 // |Summ| - контрольная сумма.
 // формат отправляемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...___...|Dn|...|Summ|
 // |D0|,|D1|,...,|Dn| - данные, 8 байт 
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
 
 //считать из внешней памяти eeprom все данные телеметрии
 //отправить всё по уарт пакетами, кол-во пакетов = адрес последнего записанного байта разделить на 8;
 //формат принимаемого пакета данных(RECEIVE):
 // |S|...|Pkt_Size|...|Pkt_number|...|Type|...|Summ|
 // |S| = 0x53 - стартовый байт;
 // |Pkt_Size| - общее количество байт в пакете;
 // |Pkt_number| = 0x0B - идентификационный номер пакета;
 // |Type| - тип пакета, ожидаемого приёмником;
 // - 0x01 - начать отправку записанных в памяти данных передачей служебных данных и обнулить счетчик адреса в памяти для считывания и отправки;
 // - 0х02 - передать 8 байт данных из памяти не изменяя текущего значения счетчика;
 // - 0х03 - передать следующие 8 байт данных из памяти (перед считыванием данных увеличить счетчик на 8, считанные данные передать по УАРТ);
 // - 0х04 - прервать передачу данных и обнулить счётик адреса;
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
 // - 0х03 - приёмник передал все данные, то есть счётчик указателя памяти на данные для считывания и передачи по УАРТ стал равен текущему адресу, по
 // которому будет записана следующая порция данных времени и температуры или сам приёмник прервал передачу данных;
 //   |Depend on type| = Nothing;
 // |Summ| - контрольная сумма.
 void ReadFullTelemetryByPackets(uint8_t *data) {
	 uint8_t packet_type = *data	;
	 
	 switch(packet_type) {
		case 0x01:
			//обнуление переменной, содержащей адрес внешней EEPROM, с которого будут считываться данные для отправки
			send_address = 0	;
			SendServiceData()	;
			break;
			
		case 0x02:		
			SendDataPacket();
			break;
		
		case 0x03:
			SendNextDataPacket()	;	
			break;
		
		case 0x04:
			StopDataTransfer()	;	
			break;
			
		default:
			
			break;		
	 }
 }
 
 //отправка всей необходимой служебной информации (количество байт в памяти, количество записанных 
 //в памяти датчиков, их идентификационные номера и т.д.) перед отправкой данных из внешней памяти 
 void SendServiceData(void) {
	 uint8_t read_status, status = 0xFF	;
	 unsigned char address[4]	;
	 uint8_t sensors_number;
	 uint16_t eep_address	;	 
	 unsigned char check_sum = 0	;
	 unsigned char packet_size = 0	;
	 	 
	 //считывание текущего адреса внешней EEPROM
	 read_status = Read_reserved_string_mega_eeprom(2, address, 4)	;			//считать адрес из внутренней eeprom
	 status &= read_status	;
	 //считать количество датчиков, id которых записано во внутреннюю eeprom
	 read_status = Read_info_byte_mega_eeprom(0, &sensors_number)	;	//считать количество датчиков из внутренней eeprom
	 status &= read_status	;
	 packet_size = 9 + sensors_number*11	;
	 
	 if((status != 0) &&
			(sensors_number > 0) &&
			(sensors_number < 8)) {
		unsigned char letter_code[2]	;
		unsigned char sensor_id[8]	;
		unsigned char output[packet_size + 1]	;		
		
		//записываем по адресу указателя заголовок пакета - стартовый байт, размер, номер и тип пакета				 
		output[0] = 0x53	;
		//packet_size = 9 + sensors_number*11	;
		output[1] = packet_size	;
		check_sum += packet_size	;
		
		output[2] = 0x0B	;
		check_sum += 0x0B	;
		
		output[3] = 0x01	;
		check_sum += 0x01	;
		
		//текщий адрес внешней EEPROM
		for (int8_t i = 3; i >= 0; i--) {
			output[7 - i] = address[i]	;					//адрес на 4,5,6,7 элемент массива
			check_sum += address[i]	;
		}
		//количество записанных во внутренню EEPROM id датчиков
		output[8] = sensors_number	;
		check_sum += sensors_number	;
		
		for (int8_t i = 1; i <= sensors_number; i++) {
			//локальный id i-го датчика(соответствует порядковому номеру записи во внутренней памяти)
			output[9 + (i - 1)*11] = i	;
			check_sum += i	;
			//два байта кода буквы, ассоциированного с i-ым датчиком
			eep_address = 136 + (i - 1)*2	;
			read_status = Read_reserved_string_mega_eeprom(eep_address, letter_code, 2)	;
			status &= read_status	;
			output[10 + (i - 1)*11] = letter_code[1]	;
			check_sum += letter_code[1]	;
			output[11 + (i - 1)*11] = letter_code[0]	;
			check_sum += letter_code[0]	;			
			//8 байт с id i-го датчика
			eep_address = i*8	;
			read_status = Read_reserved_string_mega_eeprom(eep_address, sensor_id, 8)	;
			status &= read_status	;
			
			for (int8_t j = 0; j < 8; j++) {				
				output[12 + (i - 1)*11 + j] = sensor_id[j]	;
				check_sum += sensor_id[j]	;
			}
		}
		output[packet_size] = check_sum	;
		if (status != 0) {
			USART_SendArray(output, (packet_size + 1))	;
		}
	 }	 
 }
 
 //отправка пакета с данными, считанными из внешней памяти EEPROM
 void SendDataPacket(void) {
	uint8_t buf[12], output_buf[13]	;
	uint8_t summa	;	
	
	Read_string_ext_eeprom(send_address, buf, 8)	;
	//ответный пакет для отправки по UART
	output_buf[0] = 0x53	;
	output_buf[1] = 0x0C	;
	summa = output_buf[1]	;
	output_buf[2] = 0x0B	;
	summa += output_buf[2]	;
	output_buf[3] = 0x02	;
	summa += output_buf[3]	;
	
	//нужные данные содержатся в массиве buf начиная с индекса 4
	for (uint8_t j = 4; j < 12; j++)
	{
		output_buf[j] = buf[j]	;
		summa += output_buf[j]	;
	}
	output_buf[12] = summa	;
	USART_SendArray(output_buf, 13)	;
 }
 
 //Отправить следующий пакет, увеличив счетчик адреса считываемых данных на 8 и сравнить его с текущим адресом внешней EEPROM для записи телеметрии,
 //и если текущий адрес для записи в память больше, чем адрес в памяти для отправления, начать передачу, в противном случае отправить пакет с информацией 
 //о том, что все данные переданы
 void SendNextDataPacket(void) {
	 uint32_t current_address	;
	 unsigned char address[4]	;
	 uint8_t read_status	;
	 
	 //увеличить счётчик адреса считывания данных
	 send_address += 8	;
	 
	 //считывание текущего адреса внешней EEPROM для записи телеметрии
	 read_status = Read_reserved_string_mega_eeprom(2, address, 4)	;
	 if(read_status) {
		 Convert4_to_1(address, &current_address)	;
		 if (current_address > send_address){
			 SendDataPacket()	;
		 } else {
			 StopDataTransfer()	;
		 }
	 }
 }
 
 //Пакет с информацией о том, что все данные считаны и передача данных будет прекращена
 void StopDataTransfer(void) {	 
	 unsigned char output[5]	;
	 unsigned char check_sum = 0	;
	 
	 output[0] = 0x53	;
	 output[1] = 0x04	;
	 check_sum += 0x04	;
	 
	 output[2] = 0x0B	;
	 check_sum += 0x0B	;
	 
	 output[3] = 0x03	;
	 check_sum += 0x03	;
	 
	 output[4] = check_sum	;
	 USART_SendArray(output, 5)	;
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
			output_buf[index] = allDevices[i].id[7 - j]	;
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
 
 //отправить текущую температуру с информацией о датчике (идентификационный номер, код символа, локальный id, температура)
 //для i-го датчика (порядковый номер в массиве - передаётся параметром в функцию)
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Index|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 0x0C - идентификационный номер пакета;
 // |Index| - индекс в массивах с температурой, массиве с локальным id, массиве с заводскими id.
 // |Summ| - контрольная сумма.
 //формат отправляемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Sensor_local_id|...|Sensor_id_8|...___...|Sensor_id_0|...|Sensor_Letter_1|...
 // ...|Sensor_Letter_0|...|Sensor_Temperature_1|...|Sensor_Temperature0|...|Summ|
 // |S| = 0x53 - стартовый байт; |Pkt_Size| - общее количество байт в пакете; |Pkt_number| = 0x0C - идентификационный номер пакета;
 // |Sensor_local_id| - локальный id датчика;
 // |Sensor_id_N| - N-ый байт идентификационного номера датчика,
 // |Sensor_Letter_1|...|Sensor_Letter_0| - код символа, ассоциированного с датчиком,
 // |Sensor_Temperature_1|...|Sensor_Temperature0| - два байта с текущей температурой, измеренной датчиком
 // |Summ| - контрольная сумма пакета.
 void SendSensorInfo(uint8_t *data, unsigned char sensors_num, unsigned int * temperature, uint8_t * local_id, OWI_device * allDevices) {
	  //объявление буффера, куда складываются данные для отправки по УАРТ
	  uint8_t buffer_size = 17	;
	  uint8_t output_buf[buffer_size]	;
	  uint8_t sensor_index = *data	;
	  uint16_t address, temp	;
	  uint8_t letter[2]	;
	  uint8_t templ	;
	  uint8_t temph	;
	  uint8_t summa = 0	;
	  
	  if(sensor_index < sensors_num) {
			//заголовок пакета
			output_buf[0] = 0x53	;
			output_buf[1] = buffer_size - 1	;
			output_buf[2] = 0x0C	;
			//локальный id
			output_buf[3] = local_id[sensor_index]	;
			////id датчика
			for(uint8_t j = 0; j < SENSOR_ID_SIZE; j++) {
				output_buf[4 + j] = allDevices[sensor_index].id[7 - j]	;
			}
			//код буквы, ассоциированной с датчиком
			address = 136 + (local_id[sensor_index] - 1)*2	;
			Read_reserved_string_mega_eeprom(address, letter, 2)	;
			output_buf[12] = letter[1]	;
			output_buf[13] = letter[0]	;
			//температура
			temp = temperature[sensor_index]	;
			templ = temp	;
			temph = temp>>8	;
			output_buf[14] = temph	;
			output_buf[15] = templ	;
			//вычисление контрольной суммы
			for(uint8_t i = 1; i < (buffer_size - 1); i++) {
				summa = summa + output_buf[i]	;
			}
			output_buf[buffer_size - 1] = summa	;
			//отправка по УАРТ
			USART_SendArray(output_buf, buffer_size)	;
	  }
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
 
 //Очистить внешнюю память термометра и установить её текущий адрес на 0
 //формат принимаемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Summ|
 // |S| = 0x53 - стартовый байт; 
 // |Pkt_Size| - общее количество байт в пакете; 
 // |Pkt_number| = 0x0D - идентификационный номер пакета;
 // |Summ| - контрольная сумма.
 //формат отправляемого пакета данных:
 // |S|...|Pkt_Size|...|Pkt_number|...|Result|...|Summ|
 // |S|, |Pkt_Size|, |Pkt_number|, |Summ| - то же, что и в принимаемом;
 // Result = 0xFF - сигнализирует об успешном завершении операции.
 void ClearThermometerExtMemory() {
	 
	 uint8_t output_buf[5]	;
	 uint8_t summa = 0	;
	 uint8_t *data = 0	;
	 
	 EEPROM_chip_erase()	;
	 Write_reserved_string_mega_eeprom(2, data, 4)	;
	 
	 output_buf[0] = 0x53	;
	 output_buf[1] = 0x04	;
	 summa = 0x04	;
	 output_buf[2] = 0x0D	;
	 summa += output_buf[2]	;
	 output_buf[3] = 0xFF	;
	 summa += output_buf[3]	;
	 output_buf[4] = summa	;
	 
	 USART_SendArray(output_buf, 5)	;
 }