//***************************************************************************
//
//  Author(s)...: Pashgan    http://ChipEnable.Ru   
//
//  Target(s)...: 
//
//  Compiler....: 
//
//  Description.: ���������� ���������������� ����
//
//  Data........:  
//
//***************************************************************************
#ifndef CONFIG_H
#define CONFIG_H

#define F_CPU 8000000UL

//��� ��������� � ���� ������ ������� DS18B20
#define DS18B20_FAMILY_ID                0x28
#define DS18B20_CONVERT_T                0x44
#define DS18B20_READ_SCRATCHPAD          0xbe
#define DS18B20_WRITE_SCRATCHPAD         0x4e
#define DS18B20_COPY_SCRATCHPAD          0x48
#define DS18B20_RECALL_E                 0xb8
#define DS18B20_READ_POWER_SUPPLY        0xb4

//�����, � �������� ���������� 1Wire ����������
#define BUS   OWI_PIN_7

//���������� ��������� �� ���� 1Wire
#define MAX_DEVICES       0x09


#endif //CONFIG_H