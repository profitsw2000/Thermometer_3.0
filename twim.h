//***************************************************************************
//
//  Author(s)...: ����� ������  http://ChipEnable.Ru   
//
//  Target(s)...: mega16
//
//  Compiler....: GCC
//
//  Description.: ������� �������� TWI ����������. 
//                ��� ������� �� Atmel`������ ����� - AVR315.
//
//  Data........: 13.11.13
//
//***************************************************************************
#ifndef TWIM_H
#define TWIM_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "config.h"

/****************************************************************************
  ��������� ������
****************************************************************************/

///*���� �� ���������� �������� 
//�������, ���������� �� �����*/
//#ifndef F_CPU
   //#define F_CPU  8000000UL
//#endif

/*������ ������ TWI ������*/
#define TWI_BUFFER_SIZE  9      

/****************************************************************************
  ��������� ���� TWI ������ 
****************************************************************************/

/*����� ��������� ���� */                    
#define TWI_START                  0x08  // ��������� START ������������ 
#define TWI_REP_START              0x10  // ��������� ��������� START ������������ 
#define TWI_ARB_LOST               0x38  // ��� ������� ��������� 

/*��������� ���� �������� �����������*/                
#define TWI_MTX_ADR_ACK            0x18  // ��� ������� ����� SLA+W � �������� �������������
#define TWI_MTX_ADR_NACK           0x20  // ��� ������� ���� SLA+W � �� �������� �������������
#define TWI_MTX_DATA_ACK           0x28  // ��� ������� ���� ������ � �������� �������������  
#define TWI_MTX_DATA_NACK          0x30  // ��� ������� ���� ������ � �� �������� �������������

/*��������� ���� �������� ���������*/ 
#define TWI_MRX_ADR_ACK            0x40  // ��� ������� ����� SLA+R � �������� ������������ 
#define TWI_MRX_ADR_NACK           0x48  // ��� ������� ����� SLA+R � �� �������� ������������� 
#define TWI_MRX_DATA_ACK           0x50  // ���� ������ ������ � �������� �������������  
#define TWI_MRX_DATA_NACK          0x58  // ��� ������ ���� ������ ��� �������������  

/*������ ��������� ����*/
#define TWI_NO_STATE               0xF8  // �������������� ���������; TWINT = �0�
#define TWI_BUS_ERROR              0x00  // ������ �� ���� ��-�� ����������� ��������� ����� ��� ����

/*���������������� ����*/
#define TWI_SUCCESS                0xff

/****************************************************************************
  ����������� ��������
****************************************************************************/

#define TWI_READ_BIT     0       // ������� R/W ���� � �������� ������
#define TWI_ADR_BITS     1       // ������� ������ � �������� ������
#define TRUE             1
#define FALSE            0

#define DS1307_ADR			104

/****************************************************************************
  ���������������� �������
****************************************************************************/

/*������������� � ��������� ������� SCL �������*/
uint8_t TWI_MasterInit(uint16_t fr);

/*�������� ������*/
void TWI_SendData(uint8_t *msg, uint8_t msgSize);

/*�������� �������� ������*/
uint8_t TWI_GetData(uint8_t *msg, uint8_t msgSize);

/*����� ������ TWI ������*/
uint8_t TWI_GetState(void);

/*�������� �������� �������*/
void Check_time(void);

#endif //TWIM_H