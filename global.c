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

//��������� (�������������) ���������
//������ ������� ����������� ������ �� ���������, ������� �� ������������� � ������ ������
void Init_peripheral(void)
{
	SPI_Init()	;
	Timer1_Init()	;
	ADC_Init()	;
	Ports_Init()	;
}

//��������� ������� 1 ��� ���������� �������� �����������
void Timer1_Init(void)
{
	uint8_t temp	;
	
	TCCR1A = (1<<COM1A1) | (1<<COM1A0) | (1<<WGM11) | (1<<WGM10)	;		//����� fast pwm, ���������� ���. 0 �� ������ OC1A ��� TCNT = 0, 
	TCCR1B = (1<<WGM12) | (1<<CS10);										//���������� ���. 1 ��� ���������� � OCR1A, ������������ - 256
	OCR1A = 0x80	;														//������� ��� ��������� �������
	temp = TIMSK	;
	temp |= (1<<TOIE1)	;
	TIMSK = temp	;
}

//��������� ��� - ��������� ���������� �� ������������� ��� ����������� ������� ��������� ���������
void ADC_Init(void)
{
	ADMUX = (1<<REFS0) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0)	;			//������� ���������� - ���������� �������, 7-�� ����� ���
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0)	;		//������������ - 128, ������ - �������
}


void Ports_Init(void)
{
	uint8_t temp	;
	
	temp = OC1A_DDR	;					//�������� ��� OC1A �� �����
	temp |= (1<<OC1A_BIT)	;
	OC1A_DDR = temp	;

	//���������
	temp = DDRC	;
	temp |= (1<<LED)	;
	DDRC = temp;
	temp = PORTC	;
	temp |= (0<<LED)	;
	PORTC = temp	;
	//������
	temp = DDRA	;
	temp &= ~(1<<VCC_BTN)	;
	DDRA = temp	;
	temp = PORTA	;
	temp &= ~(1<<VCC_BTN)	;
	PORTA = temp	;
}

/*****************************************************************************
*   Function name :   DS18B20_ReadTemperature
*   Returns :       ���� - READ_CRC_ERROR, ���� ��������� ������ �� ������ ��������
*                          READ_SUCCESSFUL, ���� ������ ������ ��������    
*   Parameters :    bus - ����� ����������������, ������� ��������� ���� 1WIRE ����
*                   *id - ��� ������� �� 8-�� ���������, � ������� ��������
*                         ����� ������� DS18B20
*                   *temperature - ��������� �� ����������� ��������� ����������
*                                � ������� ����� ��������� ���������� ��. �����������
*   Purpose :      �������� ������ DS18B20, ���� ������� �� �������������� �����������
*                  ����, ��������� ��� ������ - scratchpad, ��������� CRC,
*                  ��������� �������� ����������� � ����������, ���������� ��� ������             
*****************************************************************************/
unsigned char DS18B20_ReadTemperature(unsigned char bus, unsigned char * id, unsigned int* temperature)
{
    unsigned char scratchpad[9];
    unsigned char i;
  
    /*������ ������ ������
    ������� ��� ��������� 1-Wire ���������� �� ����
    ������ ������� - ����� �������������� */
    OWI_DetectPresence(bus);
    OWI_MatchRom(id, bus);
    OWI_SendByte(DS18B20_CONVERT_T ,bus);

    /*����, ����� ������ �������� ��������������*/ 
    while (!OWI_ReadBit(bus));

    /*������ ������ ������
    ������� ��� ��������� 1-Wire ���������� �� ����
    ������� - ������ ���������� ������
    ����� ��������� ���������� ������ ������� � ������
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

//������ � eeprom, � ������ �������������, id ��������� �������� � ���������� ���������� id
void ID_Registration(unsigned char sensors_num, unsigned char crcFlag, uint8_t *local_id, OWI_device *allDevices)
{
	if ((sensors_num != 0) && (crcFlag != SEARCH_CRC_ERROR))
	{
		uint8_t status, infobyte, number	;
		
		//���� ������� �������, �� ������� ����������� ������������� �� ��� ������ � eeprom.
		//���� ��� ����������� ������ � eeprom, �� �� ������ 1 ����� �������� ������ 0x77
		//�����: ������ � ������ � eeprom ������������ � ������� ��������������� (��. �������� �������)
		status = Read_info_byte_mega_eeprom(1, &infobyte)	;
		if (status)
		{
			if (infobyte == 0x77)
			{
				//���� � ������ ��� �������� ���. ������ ��������
				Read_info_byte_mega_eeprom(0, &number)	;
				if (status)
				{
					for (uint8_t i = 0; i<sensors_num; i++)									//��������� ���. ������� ��������� ��������
					{
						local_id[i] = 0	;
						for(uint8_t j = 1; j <= number; j++)								//� ���. �������� ��� ���������� � ���������� ���������� ������ ������� ��������� �������
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
					//���� �� ������� ��������� ������� ���� � ���������� ���������� ������
					//�������� ���� ���������� � ����-������� ������
					infobyte = 0	;
					
					Write_info_byte_mega_eeprom(0, infobyte)	;
					Write_info_byte_mega_eeprom(1, infobyte)	;
				}
			}
			else
			{
				//���� ��� ������ ������ � ������, �� ������ �������� ��� ��������� ������ ��������� �������� � ������
				infobyte = 0x77	;				
				unsigned char eep[4] = {0, 0, 0, 0}	;
				
				Write_info_byte_mega_eeprom(0, sensors_num)	;
				Write_info_byte_mega_eeprom(1, infobyte)	;
				Write_reserved_string_mega_eeprom(2, eep, 4)	;				//����� �� ������� ������, � �������� �������� ������ 
				
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
			
			//���� ��� ������ ��������� ������, �� ������ �������� ��� ��������� ������ ��������� �������� � ������
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

//��������� ������� � ������� �������������
//���������� ���������� �� ��
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

//�������������� 4-� ������������ ���������� � ���� 4-� ������� 
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

 //�������������� ����� 4-� ������� ���������� � 4 �����������
 void Convert1_to_4(uint8_t *data, uint32_t *dword)
 {
	 uint32_t temp	;

	temp = *dword	;
	 *data++ = temp	;
	 *data++ = (temp<<8)	;
	 *data++ = (temp<<16)	;
	 *data++ = (temp<<24)	;
 }
 
 //�������� ���������� ���������� ������ �� ������� eeprom
 //���� ��������� ������ ������� ������ �������� �� ������ ����������, 
 //�� ���������� �������� ����������, ��� ������ ������, ��� ���� ��������.
 //������ ����������� ������������ �������� ����� � ������� �������� ���������:
 //���� ������� �����(� ������ � ���������� ������) ������ �������� MEMORY_INDICATION_TRESHOLD,
 //�� ��������� ������ �� ����������. ���� ������� ����� �������� �������� MEMORY_INDICATION_TRESHOLD,
 //�� ��������� ���������� �� ����� TIM0_FREQENCY/LED_LIGHTNING_COEF (� ����������� ������������ ������� 0),
 //�.�. �� 1/LED_LIGHTNING_COEF �������. ����� �� ����� ������� �� ���������� ���������� ������ � ������������
 //��������� �������: ���������� ������ ������� �� MEMORY_INDICATION_FREQ_NUMBER ��������. 
 //����������� ���������� ������������� �������� (� +1) � ���������� �� TIM0_FREQENCY/LED_LIGHTNING_COEF.
 //��������� ����� ����� ����� ������� ������� �� ��������� � ����������� ������������ ������� 0.
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
 
 //�������� ��� ������� ��� ����������� �� 16-���������� ����������
 //��� ��������� �� ������ eeprom 
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
 
 //���������� ������� � ������ �� ������� ������ �������� ����������� � ������� ��� ������� ������� (������ 10 �����)
 void WriteTemperatureTime(uint8_t *local_id, uint8_t sensors_num, unsigned int *temperature, uint8_t *flag)
 {
	uint8_t buf[8], minutes	;
			
	//������� � ������� �����.
	//������� ���������� ����� ��������, � �������� ������� ���������� ������
	buf[0] = (DS1307_ADR<<1)|0	;						//����� ����������
	buf[1] = 0	;										//�����, � �������� ����� ���������
	TWI_SendData(buf, 2)	;
	//����������
	buf[0] = (DS1307_ADR<<1)|1	;
	TWI_SendData(buf, 8)	;
	TWI_GetData(buf, 8)	;					//������ ����� � ���� ��������� � ������ buf
	minutes = buf[2]%(0x10)	;				//� buf[2] ��������� �������� �����, ���� ��� ������ 10, �� ��������� ������� ����� ����� 0
	
	if (minutes == 0)
	{
		if (*flag == 0)
		{
			*flag = 0xFF	;
			for(uint8_t i = 0; i < sensors_num; i++)
			{
				uint8_t status	;
				unsigned char eep[4] = {0, 0, 0, 0}	;
				
				status  = Read_reserved_string_mega_eeprom(2, eep, 4)	;			//������� ����� �� ���������� eeprom, �� �������� ����� ������������ ������
				if (status)
				{
					uint32_t address	;
					uint8_t wrbuf[8]	;
					uint8_t templ, temph	;
					unsigned int temp16	; 
					
					address = eep[3]	;											//�������� ����� �� 32-������ ���������� � 4 8-������ 
					address = (address<<8) | eep[2]	;
					address = (address<<8) | eep[1]	;
					address = (address<<8) | eep[0]	;
					
					//�������� ������ ��� ������ � ������ � ������ �� 8 ����
					wrbuf[0] = *(local_id + i)	;			//��������� ����� �������
					wrbuf[1] = buf[7]	;						//���
					wrbuf[2] = buf[6]	;					//�����
					wrbuf[3] = buf[5]	;					//����
					wrbuf[4] = buf[3]	;					//����
					wrbuf[5] = buf[2]	;					//������
					
					temp16 = *(temperature + i)	;			//����������� 
					templ = temp16	;
					temp16 = temp16>>8	;
					temph = temp16	;
					wrbuf[6] = temph	;
					wrbuf[7] = templ	;
					
					Write_string_ext_eeprom(address, wrbuf, 8)	;					//�������� ������ �� ������� ������
					address = address + 8	;										//��������� ����� �� 8
					
					eep[0] = address	;
					eep[1] = address>>8	;
					eep[2] = address>>16	;
					eep[3] = address>>24	;
					Write_reserved_string_mega_eeprom(2, eep, 4)	;
				} 
				else
				{
					Write_reserved_string_mega_eeprom(2, eep, 4)	;		//���� ��� ������ ��������� ������ - �������� ����� ������� 
				}
			}
		}
	} 
	else
	{
		*flag = 0	;
	}
 }
 
 //�������� �������, ���������� � ������� ds1307 �� ����
 //������������ ������� � ��������� � �� ����
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...___...|Dn|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 1 - ����������������� ����� ������;
 // |D0|...|D1|...___...|Dn| - ������ ������, ��� n - ���������� �������������� ������.
 // ������ ������������� ������ ������ ���������� ���������
 void SendTime(uint8_t SizePkt, uint8_t command)
 {
	 uint8_t buf[SizePkt+2]	;
	 uint8_t output_buf[SizePkt+4]	;
	 uint8_t summa = 0, temp	;
	 
	 temp = SizePkt	;
	 //������� � ������� �����
	 buf[0] = (DS1307_ADR<<1)|0	;						//����� ����������
	 buf[1] = 0	;										//�����, � �������� ����� ���������
	 TWI_SendData(buf, 2)	;
	 //����������
	 buf[0] = (DS1307_ADR<<1)|1	;
	 TWI_SendData(buf, temp+2)	;
	 TWI_GetData(buf, temp+1)	;								//������ ����� � ���� ��������� � ������ buf ������� � ������� 1
	 //������������ ������ ��� ��������
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
 
 //������ �������, ����������� �� ����
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...___...|Dn|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| - ����������������� ����� ������;
 // |D0|...|D1|...___...|Dn| - ����� ��� ������, ������� � ������.
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
 
 //������� ����� �� ���������� ������ eeprom � ���������� ����� 8 �� ���������� ������,
 //������������ ������� � ��������� � �� ����
 //������ ������������ ������ ������: 
 // |S|...|Pkt_Size|...|Pkt_number|...|Read_Size|...|Address_H|...|Address_L|...|D0|...|D1|...___...|Dn|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| - ����������������� ����� ������;
 // |Read_Size| - ���������� ����, ����������� �� ������; |Address_H|,|Address_L| - ������� � ������� ���� ������, � �������� ���������� ���������� ������
 // |D0|,|D1|,...,|Dn| - ������� ������ (���������� ���� �.�. ����� Read_Size); |Summ| - ����������� �����.
 //������ ������������� ������ ������:
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
	 //�������� ����� ��� �������� �� UART
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
 
  //������� ����� �� ������� ������ eeprom (25AA1024) � ���������� ����� 8 ���� �� ���������� ������,
  //������������ ������� � ��������� � �� ����
  //������ ������������ ������ ������:
  // |S|...|Pkt_Size|...|Pkt_number|...|Read_Size|...|Address_3|...___...|Address_0|...|D0|...|D1|...___...|Dn|...|Summ|
  // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 4 - ����������������� ����� ������;
  // |Read_Size| - ���������� ����, ����������� �� ������; |Address_3|,...,|Address_0| - ������ ����� ������, � �������� ���������� ���������� ������
  // |D0|,|D1|,...,|Dn| - ������� ������ (���������� ���� �.�. ����� Read_Size); |Summ| - ����������� �����.
  //������ ������������� ������ ������:
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
	 //�������� ����� ��� �������� �� UART
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
 
 //������� �� ���������� ������ eeprom 4 ����� �������� ������ �� ������� ������ eeprom (25AA1024) 
 //������������ ������� � ��������� � �� ����
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 5 - ����������������� ����� ������;
 // |Summ| - ����������� �����.
 // ������ ������������� ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Address_3|...___...|Address_0|...|Summ|
 // |Address_3|...___...|Address_0| - ������� ����� ��������
 void SendCurrentAddress(void)
 {
	 uint8_t status	;
	 uint8_t eep[4]	;
	 uint8_t output_buf[8]	;	 
	 
	 status  = Read_reserved_string_mega_eeprom(2, eep, 4)	;			//������� ����� �� ���������� eeprom
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
 
 //������� �� ������� ������ eeprom ��� ������ ����������
 //��������� �� �� ���� ��������, ���-�� ������� = ����� ���������� ����������� ����� ��������� �� 8;
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 6 - ����������������� ����� ������;
 // |Summ| - ����������� �����.
 // ������ ������������� ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...___...|Dn|...|Summ|
 // |D0|,|D1|,...,|Dn| - ������, 8 ���� 
 void ReadAllTelemetry(void)
 {
	 uint8_t status	;
	 uint8_t eep[4]	;
	 uint32_t pkt_nmbr = 0	;											//���������� �������
	 
	 status  = Read_reserved_string_mega_eeprom(2, eep, 4)	;			//������� ����� �� ���������� eeprom
	 if(status)
	 {
		uint32_t address	;
		
		address = eep[3]	;											//��������� 4 ����� ������ � 32-������ ����������
		address = (address<<8) | eep[2]	;
		address = (address<<8) | eep[1]	;
		address = (address<<8) | eep[0]	;
		address = address & (0xFFFFFF)	;
		
		pkt_nmbr = address/8	;										//���������� ���������� �������
	 }
	 
	 if (pkt_nmbr)
	 {
		 for (uint32_t i = 0; i < pkt_nmbr; i++)
		 {
				uint8_t buf[12], output_buf[12]	;
				uint8_t summa	;
				
				while(USART_GetTxCount());			 	 
			 	Read_string_ext_eeprom(i*8, buf, 8)	;
			 	//�������� ����� ��� �������� �� UART
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
 
 //������� �� ������� ������ eeprom ��� ������ ����������
 //��������� �� �� ���� ��������, ���-�� ������� = ����� ���������� ����������� ����� ��������� �� 8;
 //������ ������������ ������ ������(RECEIVE):
 // |S|...|Pkt_Size|...|Pkt_number|...|Type|...|Summ|
 // |S| = 0x53 - ��������� ����;
 // |Pkt_Size| - ����� ���������� ���� � ������;
 // |Pkt_number| = 0x0B - ����������������� ����� ������;
 // |Type| - ��� ������, ���������� ���������;
 // - 0x01 - ������ �������� ���������� � ������ ������ ��������� ��������� ������ � �������� ������� ������ � ������ ��� ���������� � ��������;
 // - 0�02 - �������� 8 ���� ������ �� ������ �� ������� �������� �������� ��������;
 // - 0�03 - �������� ��������� 8 ���� ������ �� ������ (����� ����������� ������ ��������� ������� �� 8, ��������� ������ �������� �� ����);
 // - 0�04 - �������� �������� ������ � �������� ������ ������;
 // |Summ| - ����������� �����.
 // ������ ������������� ������ ������(SEND):
 // |S|...|Pkt_Size|...|Pkt_number|...|Type|...|Depend on type|...|Summ|
 // |S| = 0x53 - ��������� ����;
 // |Pkt_Size| - ����� ���������� ���� � ������;
 // |Pkt_number| = 6 - ����������������� ����� ������;
 // |Type| - ��� ������������� ������;
 // - 0x01 - ����� �������� ��������� ������;
 //  |Depend on type| = |Address_3|...___...|Address_0|...|Sensors_number|...|Local_sensor0_id|...|Sensor0_Letter_1|...
 // ...|Sensor0_Letter_0|...|Sensor0_id_8|...___...|Sensor0_id_0|...______...|Local_sensorN_id|...|SensorN_Letter_1|...
 // ...|SensorN_Letter_0|...|SensorN_id_8|...___...|SensorN_id_0|
 //   -- |Address_3|...___...|Address_0| - ������� ����� ������� ������, ����� ���������� ����, ���������� � ������ �, ��������������,
 //   ������ ���������� ������������ ������;
 //	  -- |Sensors_number| - ���������� ���������� �� ���������� ������ ��������;
 //	  -- |Local_sensor0_id| - ��������� id ������ ������� ����������� � ������ �������(� ������� ��������);
 //   -- |Sensor0_Letter_1|...|Sensor0_Letter_0| - ��� ����� ��������� ���� ���������� ������� ������ ������� ����������� � ������ �������;
 //   -- |Sensor0_id_8|...___...|Sensor0_id_0| - 8 ���� � id ������ ������� ����������� � ������ �������(� ������� ��������);
 //	  -- |Sensors_number| - ���������� ���������� �� ���������� ������ ��������;
 //	  -- |Local_sensorN_id| - ��������� id N-�� ����������� � ������ �������(N = Sensors_number - 1);
 //   -- |SensorN_Letter_1|...|SensorN_Letter_0| - ��� ����� ��������� ���� ���������� ������� N-�� ����������� � ������ �������;
 //   -- |SensorN_id_8|...___...|SensorN_id_0| - 8 ���� � id N-�� ����������� � ������ �������;
 // - 0�02 - �������� 8 ���� ������;
 //   |Depend on type| = |D0|...|D1|...___...|Dn|
 //   -- |D0|,|D1|,...,|Dn| - 8 ���� ������, ��� D0 ������� � ������, �������� 8 (��� � ������ 0), � Dn - � ������, �������� n;
 // - 0�03 - ������� ������� ��� ������, �� ���� ������� ��������� ������ �� ������ ��� ���������� � �������� �� ���� ���� ����� �������� ������, ��
 // �������� ����� �������� ��������� ������ ������ ������� � ����������� ��� ��� ������� ������� �������� ������;
 //   |Depend on type| = Nothing;
 // |Summ| - ����������� �����.
 void ReadFullTelemetryByPackets(uint8_t *data) {
	 uint8_t packet_type = *data	;
	 
	 switch(packet_type) {
		case 0x01:
			//��������� ����������, ���������� ����� ������� EEPROM, � �������� ����� ����������� ������ ��� ��������
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
 
 //�������� ���� ����������� ��������� ���������� (���������� ���� � ������, ���������� ���������� 
 //� ������ ��������, �� ����������������� ������ � �.�.) ����� ��������� ������ �� ������� ������ 
 void SendServiceData(void) {
	 uint8_t read_status, status = 0xFF	;
	 unsigned char address[4]	;
	 uint8_t sensors_number;
	 uint16_t eep_address	;	 
	 unsigned char check_sum = 0	;
	 unsigned char packet_size = 0	;
	 	 
	 //���������� �������� ������ ������� EEPROM
	 read_status = Read_reserved_string_mega_eeprom(2, address, 4)	;			//������� ����� �� ���������� eeprom
	 status &= read_status	;
	 //������� ���������� ��������, id ������� �������� �� ���������� eeprom
	 read_status = Read_info_byte_mega_eeprom(0, &sensors_number)	;	//������� ���������� �������� �� ���������� eeprom
	 status &= read_status	;
	 packet_size = 9 + sensors_number*11	;
	 
	 if((status != 0) &&
			(sensors_number > 0) &&
			(sensors_number < 8)) {
		unsigned char letter_code[2]	;
		unsigned char sensor_id[8]	;
		unsigned char output[packet_size + 1]	;		
		
		//���������� �� ������ ��������� ��������� ������ - ��������� ����, ������, ����� � ��� ������				 
		output[0] = 0x53	;
		//packet_size = 9 + sensors_number*11	;
		output[1] = packet_size	;
		check_sum += packet_size	;
		
		output[2] = 0x0B	;
		check_sum += 0x0B	;
		
		output[3] = 0x01	;
		check_sum += 0x01	;
		
		//������ ����� ������� EEPROM
		for (int8_t i = 3; i >= 0; i--) {
			output[7 - i] = address[i]	;					//����� �� 4,5,6,7 ������� �������
			check_sum += address[i]	;
		}
		//���������� ���������� �� ��������� EEPROM id ��������
		output[8] = sensors_number	;
		check_sum += sensors_number	;
		
		for (int8_t i = 1; i <= sensors_number; i++) {
			//��������� id i-�� �������(������������� ����������� ������ ������ �� ���������� ������)
			output[9 + (i - 1)*11] = i	;
			check_sum += i	;
			//��� ����� ���� �����, ���������������� � i-�� ��������
			eep_address = 136 + (i - 1)*2	;
			read_status = Read_reserved_string_mega_eeprom(eep_address, letter_code, 2)	;
			status &= read_status	;
			output[10 + (i - 1)*11] = letter_code[1]	;
			check_sum += letter_code[1]	;
			output[11 + (i - 1)*11] = letter_code[0]	;
			check_sum += letter_code[0]	;			
			//8 ���� � id i-�� �������
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
 
 //�������� ������ � �������, ���������� �� ������� ������ EEPROM
 void SendDataPacket(void) {
	uint8_t buf[12], output_buf[13]	;
	uint8_t summa	;	
	
	Read_string_ext_eeprom(send_address, buf, 8)	;
	//�������� ����� ��� �������� �� UART
	output_buf[0] = 0x53	;
	output_buf[1] = 0x0C	;
	summa = output_buf[1]	;
	output_buf[2] = 0x0B	;
	summa += output_buf[2]	;
	output_buf[3] = 0x02	;
	summa += output_buf[3]	;
	
	//������ ������ ���������� � ������� buf ������� � ������� 4
	for (uint8_t j = 4; j < 12; j++)
	{
		output_buf[j] = buf[j]	;
		summa += output_buf[j]	;
	}
	output_buf[12] = summa	;
	USART_SendArray(output_buf, 13)	;
 }
 
 //��������� ��������� �����, �������� ������� ������ ����������� ������ �� 8 � �������� ��� � ������� ������� ������� EEPROM ��� ������ ����������,
 //� ���� ������� ����� ��� ������ � ������ ������, ��� ����� � ������ ��� �����������, ������ ��������, � ��������� ������ ��������� ����� � ����������� 
 //� ���, ��� ��� ������ ��������
 void SendNextDataPacket(void) {
	 uint32_t current_address	;
	 unsigned char address[4]	;
	 uint8_t read_status	;
	 
	 //��������� ������� ������ ���������� ������
	 send_address += 8	;
	 
	 //���������� �������� ������ ������� EEPROM ��� ������ ����������
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
 
 //����� � ����������� � ���, ��� ��� ������ ������� � �������� ������ ����� ����������
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
  
 //�������� �� ���������� ������ eeprom ��� (2 �����) ������� ��� ������ �� 16-���������� ���������
 //�����, �� ��������  ������������ ��� ����������� �� ������ ������������� �� ����������� �������
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Number|...|Letter1|...|Letter0|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 8 - ����������������� ����� ������;
 // |Summ| - ����������� �����; |Address_3|...___...|Address_0| - ������ ����� ������.
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
 
 //��������� ������� ����������� � ����������� � ������� (����������������� �����, ��� �������, ���������� � ���)
 //��� ���� ��������
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 9 - ����������������� ����� ������;
 // |D0|, |D1| - ������ ������
 // |Summ| - ����������� �����.
 //������ ������������� ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Sensors_number|...|Sensor0_id_8|...___...|Sensor0_id_0|...|Sensor0_Letter_1|...
 // ...|Sensor0_Letter_0|...|Sensor0_Temperature_1|...|Sensor0_Temperature0|...___...|SensorN_id_8|...___...|SensorN_id_0|...|SensorN_Letter_1|...
 // ...|SensorN_Letter_0|...|SensorN_Temperature_1|...|SensorN_Temperature0|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 9 - ����������������� ����� ������;
 // |Sensors_number| - ����� ���������� �������� �� �����
 // |SensorN_id_N| - N-�� ���� ������������������ ������ N-�� �������,
 // |SensorN_Letter_1|...|SensorN_Letter_0| - ��� �������, ���������������� � �������� N,
 // |SensorN_Temperature_1|...|SensorN_Temperature0| - ��� ����� � ������� ������������, ���������� N-�� ��������
 // |Summ| - ����������� ����� ������.
 void SendSensorsTemperature(unsigned char sensors_num, unsigned int * temperature, uint8_t * local_id, OWI_device * allDevices)
 {
	 //���������� �������, ���� ������������ ������ ��� �������� �� ����
	 //������ ������� ����� �������� �� ���������� �������� �� �����
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
	 
	 //���������� ������ ������� �������� - id, ��� ������� � ������� ������������, ���������� ��������
	 for(uint8_t i = 0; i < sensors_num; i++) {
		 uint8_t index = 4 + SENSOR_INFO_SIZE*i	;
		 
		 address = 136 + (local_id[i] - 1)*2	;
		 Read_reserved_string_mega_eeprom(address, letter, 2)	;
		 
		 temp = temperature[i]	;
		 templ = temp	;
		 temph = temp>>8	;
		 
		 ////id �������
		 for(uint8_t j = 0; j < SENSOR_ID_SIZE; j++) {
			output_buf[index] = allDevices[i].id[7 - j]	;
			index++	;
		 }
		 
		 //��� �������		 
		 output_buf[index++] = letter[1]	;
		 output_buf[index++] = letter[0]	;
		 
		 //�����������		 
		 output_buf[index++] = temph	;
		 output_buf[index++] = templ	;		   
	 }	 
	 //
	 ////���������� ����������� �����
	 for(uint8_t i = 1; i < (buffer_size - 1); i++) {
		summa = summa + output_buf[i]	;
	 }
	 output_buf[buffer_size - 1] = summa	;
	 
	 USART_SendArray(output_buf, buffer_size)	;
 }
 
 //��������� ������� ����������� � ����������� � ������� (����������������� �����, ��� �������, ��������� id, �����������)
 //��� i-�� ������� (���������� ����� � ������� - ��������� ���������� � �������)
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Index|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 0x0C - ����������������� ����� ������;
 // |Index| - ������ � �������� � ������������, ������� � ��������� id, ������� � ���������� id.
 // |Summ| - ����������� �����.
 //������ ������������� ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Sensor_local_id|...|Sensor_id_8|...___...|Sensor_id_0|...|Sensor_Letter_1|...
 // ...|Sensor_Letter_0|...|Sensor_Temperature_1|...|Sensor_Temperature0|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 0x0C - ����������������� ����� ������;
 // |Sensor_local_id| - ��������� id �������;
 // |Sensor_id_N| - N-�� ���� ������������������ ������ �������,
 // |Sensor_Letter_1|...|Sensor_Letter_0| - ��� �������, ���������������� � ��������,
 // |Sensor_Temperature_1|...|Sensor_Temperature0| - ��� ����� � ������� ������������, ���������� ��������
 // |Summ| - ����������� ����� ������.
 void SendSensorInfo(uint8_t *data, unsigned char sensors_num, unsigned int * temperature, uint8_t * local_id, OWI_device * allDevices) {
	  //���������� �������, ���� ������������ ������ ��� �������� �� ����
	  uint8_t buffer_size = 17	;
	  uint8_t output_buf[buffer_size]	;
	  uint8_t sensor_index = *data	;
	  uint16_t address, temp	;
	  uint8_t letter[2]	;
	  uint8_t templ	;
	  uint8_t temph	;
	  uint8_t summa = 0	;
	  
	  if(sensor_index < sensors_num) {
			//��������� ������
			output_buf[0] = 0x53	;
			output_buf[1] = buffer_size - 1	;
			output_buf[2] = 0x0C	;
			//��������� id
			output_buf[3] = local_id[sensor_index]	;
			////id �������
			for(uint8_t j = 0; j < SENSOR_ID_SIZE; j++) {
				output_buf[4 + j] = allDevices[sensor_index].id[7 - j]	;
			}
			//��� �����, ��������������� � ��������
			address = 136 + (local_id[sensor_index] - 1)*2	;
			Read_reserved_string_mega_eeprom(address, letter, 2)	;
			output_buf[12] = letter[1]	;
			output_buf[13] = letter[0]	;
			//�����������
			temp = temperature[sensor_index]	;
			templ = temp	;
			temph = temp>>8	;
			output_buf[14] = temph	;
			output_buf[15] = templ	;
			//���������� ����������� �����
			for(uint8_t i = 1; i < (buffer_size - 1); i++) {
				summa = summa + output_buf[i]	;
			}
			output_buf[buffer_size - 1] = summa	;
			//�������� �� ����
			USART_SendArray(output_buf, buffer_size)	;
	  }
 }
 
 
 //��������� ������� �������� ������� 
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|D0|...|D1|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 10 - ����������������� ����� ������;
 // |D0|, |D1| - ������ ������
 // |Summ| - ����������� �����.
 //������ ������������� ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Brightness_H|...|Brightness_L|...|Summ|
 // |S| = 0x53 - ��������� ����; |Pkt_Size| - ����� ���������� ���� � ������; |Pkt_number| = 10 - ����������������� ����� ������;
 // Brightness_H - ������� ���� �������� �������, Brightness_L - ������� ���� �������� �������.
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
 
 //�������� ������� ������ ���������� � ���������� � ������� ����� �� 0
 //������ ������������ ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Summ|
 // |S| = 0x53 - ��������� ����; 
 // |Pkt_Size| - ����� ���������� ���� � ������; 
 // |Pkt_number| = 0x0D - ����������������� ����� ������;
 // |Summ| - ����������� �����.
 //������ ������������� ������ ������:
 // |S|...|Pkt_Size|...|Pkt_number|...|Result|...|Summ|
 // |S|, |Pkt_Size|, |Pkt_number|, |Summ| - �� ��, ��� � � �����������;
 // Result = 0xFF - ������������� �� �������� ���������� ��������.
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