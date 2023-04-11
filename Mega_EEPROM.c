//***************************************************************************
//
//  Author(s)...: profitsw2000
//
//  Target(s)...: mega16
//
//  Compiler....: GCC
//
//  Description.: Библиотека функций для работы с памятью EEPROM AtMega16.
//
//  Data........: 20.08.2020
//
//***************************************************************************

#include "Mega_EEPROM.h"
//#include "usart.h"
/*! \brief  Запись одного байта в память eeprom.
 *
 *
 *  \param  address Данная переменная должна содержать адрес в памяти eeprom,
 *					по которому будет записываться байт данных.
 *
 *  \param  eestring   Байт данных для записи.
 *
 */
void Write_byte_mega_eeprom(uint16_t address, uint8_t eebyte)
{
	while (EECR & (1<<EEWE))	;
	EEAR = address	;
	EEDR = eebyte	;
	EECR |= (1<<EEMWE)	;
	EECR |= (1<<EEWE)	;	
	_delay_ms(10)	;
}

/*! \brief  Запись нескольких байт в память eeprom начиная с указанного адреса
 *
 *  \param  address		Данная переменная должна содержать адрес в памяти eeprom,
 *						начиная с которого будут записываться байты данных.
 *
 *  \param  eestring	Указатель на массив байтов, подлежащих записи в память eeprom.
 *
 *  \param  number		Количество байт для записи.
 *
 */
void Write_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)	
{
	for (uint8_t i = 0; i < number; i++)
	{
		uint8_t eep_byte	;
		
		eep_byte = *eestring++	;
		Write_byte_mega_eeprom(address, eep_byte)	;
		address++	;
	}
}

/*! \brief  Чтение одного байта данных из памяти eeprom.
 *
 *
 *  \param  address Адрес считываемого байта данных в памяти eeprom.
 *
 *  \retval  EEDR  Байт данных из памяти eeprom.
 * 
 */
uint8_t Read_byte_mega_eeprom(uint16_t address)	
{
	 while (EECR & (1<<EEWE));
	 EEAR = address;
	 EECR |= (1<<EERE);
	 return EEDR;
}


/*! \brief  Чтение нескольких байт данных из памяти eeprom.
 *
 *
 *  \param  address Адрес первого считываемого байта данных в памяти eeprom.
 *
 *  \param  eestring Указатель на массив данных, в который будут записываться данные из eeprom.
 *
 *  \param  address Количество байт, считываемых из eeprom.
 *
 * 
 */
void Read_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)
{	
	for (uint8_t i = 0; i < number; i++)
	{
		char *temp = 0	;
		
		temp = (eestring + i)	;
		*temp = Read_byte_mega_eeprom(address)	;
		address++	;
	}
}

/*! \brief  Запись одного байта в память eeprom с двойным резервированием. Байт данных записывается
 *			не только по указанному адресу, но также дважды по адресам, вычисленным по формуле указанной ниже.
 *
 *
 *  \param  address Данная переменная должна содержать адрес в памяти eeprom,
 *					по которому будет записываться основной байт данных. Также это значение 
 *					используется для вычисления адресов для записи двух резервных байтов.
 *
 *  \param  eebyte  Байт данных для записи.
 *
 */
void Write_info_byte_mega_eeprom(uint16_t address, uint8_t eebyte)
{
	uint16_t address1, address2	;				//адреса резервных байтов
	
	address1 = address + 21*8	;
	address2 = address + 42*8	;
	
	//запись основного байта данных
	Write_byte_mega_eeprom(address, eebyte)	;
	//запись двух резервных байтов
	Write_byte_mega_eeprom(address1, eebyte)	;
	Write_byte_mega_eeprom(address2, eebyte)	;		
}

/*! \brief  Чтение байта данных с двойным резервированием из памяти eeprom.
 *			Из памяти считываются 3 байта данных (один основной, два резервных), 
 *			и если хотя бы 2 из них равны
 *			функция возвращает ненулевое значение. Результат чтения передаётся в аргумент eebyte.
 *
 *  \param  address Адрес первого считываемого байта данных в памяти eeprom, а также значение для вычисления 
 *			адресов 2 резервных байтов данных в памяти.
 *
 *  \param  eebyte В случае успешного считывания (return = EEPROM_READ_SUCCESFUL) принимает значение одного из трёх считываемых 
 *			байтов, если его значение совпадает хотя бы с одним из двух других. Если произошла ошибка считывания, т.е. когда
 *			все три байта имеют разное значение, аргумент принимает значение первого считанного байта.
 *
 *  \retval  EEPROM_READ_SUCCESFUL в случае, если хотя бы 2 из считанных байтов равны.
 *			EEPROM_READ_ERROR в случае, если значения всех трёх байтов различны.
 * 
 */
uint8_t Read_info_byte_mega_eeprom(uint16_t address, uint8_t* eebyte)	
{
	uint16_t address1, address2	;
	char temp, temp1, temp2	;
	//вычисление адресов резервных байтов
	address1 = address + 21*8	;
	address2 = address + 42*8	;
	//считывание основного и двух резервных байтов
	temp = Read_byte_mega_eeprom(address)	;
	temp1 = Read_byte_mega_eeprom(address1)	;
	temp2 = Read_byte_mega_eeprom(address2)	;
	
	if ((temp == temp1) || (temp == temp2))
	{
		*eebyte = temp	;
		return EEPROM_READ_SUCCESFUL	;
	} 
	else if(temp1 == temp2)
	{
		*eebyte = temp1	;
		return EEPROM_READ_SUCCESFUL	;		
	}
	else
	{
		*eebyte = temp	;		
		return EEPROM_READ_ERROR	;
	}
}

/*! \brief  Запись нескольких байт в память eeprom с двойным резервированием, начиная с указанного адреса.
 *			Данные содержат id датчиков температуры (8 байт) или код для вывода на 16-сегментный индикатор (2 байта).
 *			Записываются основной массив байтов и 2 резервных. Адреса резервных байтов 
 *			получают из адреса основного по формуле.
 *
 *  \param  address		Данная переменная должна содержать адрес в памяти eeprom,
 *						начиная с которого будут записываться байты данных и с помощью которой
 *						будет вычисляться адрес резервных байтов.
 *						Значение адреса должно быть менее 168, если больше функция возвращает ошибку.
 *
 *  \param  eestring	Указатель на массив байтов, подлежащих записи в память eeprom.
 *
 *  \param  number		Количество байт для записи.
 *
 */
void Write_reserved_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)
{
	uint16_t address1, address2	;
	//вычисление адресов резервных байтов
	address1 = address + 21*8	;
	address2 = address + 42*8	;
	
	if (address > 167) return ;
		
	for (uint8_t i = 0; i < number; i++)
	{
		uint8_t eep_byte	;
				
		eep_byte = *eestring++	;
		//запись основного массива данных
		Write_byte_mega_eeprom(address, eep_byte)	;
		//запись двух резервных массивов данных
		Write_byte_mega_eeprom(address1, eep_byte)	;
		Write_byte_mega_eeprom(address2, eep_byte)	;
		address++	;
		address1++	;
		address2++	;
	}
}


/*! \brief  Чтение нескольких байт данных из памяти eeprom. Данные содержат id датчиков температуры (8 байт).
 *			Считываются данные по адресу, указанному в параметре address и два резервных массива
 *			такого же размера, адрес резервных массивов вычисляется по формуле с применение параметра address.
 *
 *  \param  address Адрес первого считываемого байта данных в памяти eeprom, также используется для определения адреса резервных данных.
 *						Значение адреса должно быть менее 168, если больше функция возвращает ошибку.
 *
 *  \param  eestring Указатель на массив данных, в который будут записываться данные из eeprom.
 *
 *  \param  number Количество байт, считываемых из eeprom.
 *
 *  \retval EEPROM_READ_SUCCESFUL в случае, если хотя бы 2 из считанных массива байтов равны.
 *			EEPROM_READ_ERROR в случае, если значения всех трёх байтов различны.
 * 
 */
uint8_t Read_reserved_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)
{
	unsigned char device[number], device1[number], device2[number]	;
	uint16_t address1, address2	;
	int d1,d2,d3	;
	
	if (address > 167) return EEPROM_READ_ERROR	;
	
	//вычисление адресов резервных массивов
	address1 = address + 21*8	;
	address2 = address + 42*8	;
	
	//считывание основного и резервных массивов
	for (uint8_t i = 0; i < number; i++)
	{
		device[i] = Read_byte_mega_eeprom(address)	;
		address++	;
		device1[i] = Read_byte_mega_eeprom(address1)	;
		address1++	;
		device2[i] = Read_byte_mega_eeprom(address2)	;
		address2++	;
	}
	
	//сравнение массивов - если хотя бы 2 из массивов равны функция возвращает значение EEPROM_READ_SUCCESFUL
	//аргументу eestring присваивается значение одного из этих массивов
	d1 = memcmp(device, device1, number)	;
	d2 = memcmp(device, device2, number)	;
	d3 = memcmp(device1, device2, number)	;
	
	if ((d1 == 0) || (d2 == 0))
	{
		for(int i = 0; i < number; i++)
		{
			char *temp = 0	;
			
			temp = (eestring + i)	;
			*temp = device[i]	;
		}
		
		return EEPROM_READ_SUCCESFUL	;
	} 
	else if (d3 == 0)
	{
		for(int i = 0; i < number; i++)
		{
			char *temp = 0	;
			
			temp = (eestring + i)	;
			*temp = device1[i]	;
		}
		
		return EEPROM_READ_SUCCESFUL	;
	}
	else
	{
		for(int i = 0; i < number; i++)
		{
			char *temp = 0	;
			
			temp = (eestring + i)	;
			*temp = device[i]	;
		}
		
		return EEPROM_READ_ERROR	;
	}
}

