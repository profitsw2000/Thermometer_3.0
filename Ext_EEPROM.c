//***************************************************************************
//
//  Author(s)...: profitsw2000
//
//  Target(s)...: mega16
//
//  Compiler....: GCC
//
//  Description.: ���������� ������� ��� ������ � ������� ������� EEPROM (�/�� 25��1024).
//
//  Data........: 20.08.2020
//
//***************************************************************************

#include "Ext_EEPROM.h"
#include "usart.h"
#include "spi.h"


/*! \brief  ������ ���������� ���� � ������ eeprom ������� � ���������� ������, 
*			����� ������� ������� ������� ���������� ������, ����� ������ - ������� ����������.
 *
 *  \param  address		������ ���������� ������ ��������� ����� � ������ eeprom,
 *						������� � �������� ����� ������������ ����� ������.
 *
 *  \param  eestring	��������� �� ������ ������, ���������� ������ � ������ eeprom.
 *
 *  \param  number		���������� ���� ��� ������.
 *
 */
void Write_string_ext_eeprom(uint32_t address, uint8_t *eestring, uint8_t number)	
{
	uint8_t wrt_pkt[number+4]	;
	uint32_t temp	;
	
	temp = WRITE_COMMAND	;
	
	address = address | (temp<<24)	;
	
	wrt_pkt[0] = (address >> 24) & 0xFF	;	
	wrt_pkt[1] = (address >> 16) & 0xFF	;
	wrt_pkt[2] = (address >> 8) & 0xFF	;
	wrt_pkt[3] = (address) & 0xFF	;
	
	for (int i = 4; i < (number + 4); i++)
	{
		wrt_pkt[i] = *eestring++	;
	}
	
	SPI_WriteByte(WREN_COMMAND)	;
	_delay_ms(1)	;
	SPI_WriteArray(number+4, wrt_pkt)	;
	_delay_ms(1)	;
	SPI_WriteByte(WRDI_COMMAND)	;
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
void Read_string_ext_eeprom(uint32_t address, uint8_t *eestring, uint8_t number)
{	
	uint32_t temp	;
	uint8_t *info	;
	
	info = eestring	;
	temp = READ_COMMAND	;
	
	address = address | (temp<<24)	;
	
	*eestring++ = (address >> 24) & 0xFF	;
	*eestring++ = (address >> 16) & 0xFF	;
	*eestring++ = (address >> 8) & 0xFF	;
	*eestring++ = (address) & 0xFF	;

	number = number + 4	;		
	SPI_ReadArray(number, info)	;	
}

/*! \brief  ������ ���������� �������� �/�� eeprom.
 *
 *
 *
 *  \return         ���������� ��������� �������
 */
uint8_t Read_status_reg_eeprom()
{	
	uint8_t info[2]	;

	info[0] = RDSR_COMMAND	;
	info[1] = RDSR_COMMAND	;
	
	SPI_ReadArray(sizeof(info), info)	;	
	return info[1]	;
}

/*! \brief  ������ � ��������� ������� �/�� eeprom.
 *
 *
 *
 *  \param  status ���� ������ ��� ������.
 */
void Write_status_reg_eeprom(uint8_t status)
{	
	uint8_t info[2]	;

	info[0] = WRSR_COMMAND	;
	info[1] = status	;
	
	SPI_WriteArray(sizeof(info), info)	;	
}

/*! \brief  �������� �������� ������ eeprom.
 *
 *  \param  address		������ ���������� ������ ��������� ����� � ������ eeprom,
 *						� �������� ��������, ������� ���������� ��������.
 *
 *
 */
void EEPROM_page_erase(uint32_t address)	
{
	uint32_t temp	;
	uint8_t wrt_pkt[4]	;
	
	temp = PE_COMMAND	;
	
	address = address | (temp<<24)	;
	
	wrt_pkt[0] = (address >> 24) & 0xFF	;	
	wrt_pkt[1] = (address >> 16) & 0xFF	;
	wrt_pkt[2] = (address >> 8) & 0xFF	;
	wrt_pkt[3] = (address) & 0xFF	;
	
	SPI_WriteByte(WREN_COMMAND)	;
	_delay_ms(1)	;
	SPI_WriteArray(4, wrt_pkt)	;
}

/*! \brief  �������� ������ ������ eeprom.
 *
 *  \param  address		������ ���������� ������ ��������� ����� � ������ eeprom,
 *						� �������� ��������, ������� ���������� ��������.
 *
 *
 */
void EEPROM_sector_erase(uint32_t address)	
{
	uint32_t temp	;
	uint8_t wrt_pkt[4]	;
	
	temp = SE_COMMAND	;
	
	address = address | (temp<<24)	;
	
	wrt_pkt[0] = (address >> 24) & 0xFF	;	
	wrt_pkt[1] = (address >> 16) & 0xFF	;
	wrt_pkt[2] = (address >> 8) & 0xFF	;
	wrt_pkt[3] = (address) & 0xFF	;
	
	SPI_WriteByte(WREN_COMMAND)	;
	_delay_ms(1)	;
	SPI_WriteArray(4, wrt_pkt)	;
}

/*! \brief  �������� ��� ������ eeprom.
 *
 *
 *
 */
void EEPROM_chip_erase(void)	
{		
	SPI_WriteByte(WREN_COMMAND)	;
	_delay_ms(1)	;
	SPI_WriteByte(CE_COMMAND)	;
}