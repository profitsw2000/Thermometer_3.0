//***************************************************************************
//
//  Author(s)...: profitsw2000
//
//  Target(s)...: mega16
//
//  Compiler....: GCC
//
//  Description.: ���������� ������� ��� ������ � ������� EEPROM AtMega16.
//
//  Data........: 20.08.2020
//
//***************************************************************************

#include "Mega_EEPROM.h"
//#include "usart.h"
/*! \brief  ������ ������ ����� � ������ eeprom.
 *
 *
 *  \param  address ������ ���������� ������ ��������� ����� � ������ eeprom,
 *					�� �������� ����� ������������ ���� ������.
 *
 *  \param  eestring   ���� ������ ��� ������.
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

/*! \brief  ������ ���������� ���� � ������ eeprom ������� � ���������� ������
 *
 *  \param  address		������ ���������� ������ ��������� ����� � ������ eeprom,
 *						������� � �������� ����� ������������ ����� ������.
 *
 *  \param  eestring	��������� �� ������ ������, ���������� ������ � ������ eeprom.
 *
 *  \param  number		���������� ���� ��� ������.
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

/*! \brief  ������ ������ ����� ������ �� ������ eeprom.
 *
 *
 *  \param  address ����� ������������ ����� ������ � ������ eeprom.
 *
 *  \retval  EEDR  ���� ������ �� ������ eeprom.
 * 
 */
uint8_t Read_byte_mega_eeprom(uint16_t address)	
{
	 while (EECR & (1<<EEWE));
	 EEAR = address;
	 EECR |= (1<<EERE);
	 return EEDR;
}


/*! \brief  ������ ���������� ���� ������ �� ������ eeprom.
 *
 *
 *  \param  address ����� ������� ������������ ����� ������ � ������ eeprom.
 *
 *  \param  eestring ��������� �� ������ ������, � ������� ����� ������������ ������ �� eeprom.
 *
 *  \param  address ���������� ����, ����������� �� eeprom.
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

/*! \brief  ������ ������ ����� � ������ eeprom � ������� ���������������. ���� ������ ������������
 *			�� ������ �� ���������� ������, �� ����� ������ �� �������, ����������� �� ������� ��������� ����.
 *
 *
 *  \param  address ������ ���������� ������ ��������� ����� � ������ eeprom,
 *					�� �������� ����� ������������ �������� ���� ������. ����� ��� �������� 
 *					������������ ��� ���������� ������� ��� ������ ���� ��������� ������.
 *
 *  \param  eebyte  ���� ������ ��� ������.
 *
 */
void Write_info_byte_mega_eeprom(uint16_t address, uint8_t eebyte)
{
	uint16_t address1, address2	;				//������ ��������� ������
	
	address1 = address + 21*8	;
	address2 = address + 42*8	;
	
	//������ ��������� ����� ������
	Write_byte_mega_eeprom(address, eebyte)	;
	//������ ���� ��������� ������
	Write_byte_mega_eeprom(address1, eebyte)	;
	Write_byte_mega_eeprom(address2, eebyte)	;		
}

/*! \brief  ������ ����� ������ � ������� ��������������� �� ������ eeprom.
 *			�� ������ ����������� 3 ����� ������ (���� ��������, ��� ���������), 
 *			� ���� ���� �� 2 �� ��� �����
 *			������� ���������� ��������� ��������. ��������� ������ ��������� � �������� eebyte.
 *
 *  \param  address ����� ������� ������������ ����� ������ � ������ eeprom, � ����� �������� ��� ���������� 
 *			������� 2 ��������� ������ ������ � ������.
 *
 *  \param  eebyte � ������ ��������� ���������� (return = EEPROM_READ_SUCCESFUL) ��������� �������� ������ �� ��� ����������� 
 *			������, ���� ��� �������� ��������� ���� �� � ����� �� ���� ������. ���� ��������� ������ ����������, �.�. �����
 *			��� ��� ����� ����� ������ ��������, �������� ��������� �������� ������� ���������� �����.
 *
 *  \retval  EEPROM_READ_SUCCESFUL � ������, ���� ���� �� 2 �� ��������� ������ �����.
 *			EEPROM_READ_ERROR � ������, ���� �������� ���� ��� ������ ��������.
 * 
 */
uint8_t Read_info_byte_mega_eeprom(uint16_t address, uint8_t* eebyte)	
{
	uint16_t address1, address2	;
	char temp, temp1, temp2	;
	//���������� ������� ��������� ������
	address1 = address + 21*8	;
	address2 = address + 42*8	;
	//���������� ��������� � ���� ��������� ������
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

/*! \brief  ������ ���������� ���� � ������ eeprom � ������� ���������������, ������� � ���������� ������.
 *			������ �������� id �������� ����������� (8 ����) ��� ��� ��� ������ �� 16-���������� ��������� (2 �����).
 *			������������ �������� ������ ������ � 2 ���������. ������ ��������� ������ 
 *			�������� �� ������ ��������� �� �������.
 *
 *  \param  address		������ ���������� ������ ��������� ����� � ������ eeprom,
 *						������� � �������� ����� ������������ ����� ������ � � ������� �������
 *						����� ����������� ����� ��������� ������.
 *						�������� ������ ������ ���� ����� 168, ���� ������ ������� ���������� ������.
 *
 *  \param  eestring	��������� �� ������ ������, ���������� ������ � ������ eeprom.
 *
 *  \param  number		���������� ���� ��� ������.
 *
 */
void Write_reserved_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)
{
	uint16_t address1, address2	;
	//���������� ������� ��������� ������
	address1 = address + 21*8	;
	address2 = address + 42*8	;
	
	if (address > 167) return ;
		
	for (uint8_t i = 0; i < number; i++)
	{
		uint8_t eep_byte	;
				
		eep_byte = *eestring++	;
		//������ ��������� ������� ������
		Write_byte_mega_eeprom(address, eep_byte)	;
		//������ ���� ��������� �������� ������
		Write_byte_mega_eeprom(address1, eep_byte)	;
		Write_byte_mega_eeprom(address2, eep_byte)	;
		address++	;
		address1++	;
		address2++	;
	}
}


/*! \brief  ������ ���������� ���� ������ �� ������ eeprom. ������ �������� id �������� ����������� (8 ����).
 *			����������� ������ �� ������, ���������� � ��������� address � ��� ��������� �������
 *			������ �� �������, ����� ��������� �������� ����������� �� ������� � ���������� ��������� address.
 *
 *  \param  address ����� ������� ������������ ����� ������ � ������ eeprom, ����� ������������ ��� ����������� ������ ��������� ������.
 *						�������� ������ ������ ���� ����� 168, ���� ������ ������� ���������� ������.
 *
 *  \param  eestring ��������� �� ������ ������, � ������� ����� ������������ ������ �� eeprom.
 *
 *  \param  number ���������� ����, ����������� �� eeprom.
 *
 *  \retval EEPROM_READ_SUCCESFUL � ������, ���� ���� �� 2 �� ��������� ������� ������ �����.
 *			EEPROM_READ_ERROR � ������, ���� �������� ���� ��� ������ ��������.
 * 
 */
uint8_t Read_reserved_string_mega_eeprom(uint16_t address, uint8_t *eestring, uint8_t number)
{
	unsigned char device[number], device1[number], device2[number]	;
	uint16_t address1, address2	;
	int d1,d2,d3	;
	
	if (address > 167) return EEPROM_READ_ERROR	;
	
	//���������� ������� ��������� ��������
	address1 = address + 21*8	;
	address2 = address + 42*8	;
	
	//���������� ��������� � ��������� ��������
	for (uint8_t i = 0; i < number; i++)
	{
		device[i] = Read_byte_mega_eeprom(address)	;
		address++	;
		device1[i] = Read_byte_mega_eeprom(address1)	;
		address1++	;
		device2[i] = Read_byte_mega_eeprom(address2)	;
		address2++	;
	}
	
	//��������� �������� - ���� ���� �� 2 �� �������� ����� ������� ���������� �������� EEPROM_READ_SUCCESFUL
	//��������� eestring ������������� �������� ������ �� ���� ��������
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

