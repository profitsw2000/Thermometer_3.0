//***************************************************************************
//
//  Author(s)...: Pashgan    http://ChipEnable.Ru   
//
//  Target(s)...: ATMega8535
//
//  Compiler....: WINAVR
//
//  Description.: USART/UART. ���������� ��������� �����
//
//  Data........: 3.01.10 
//
//***************************************************************************
#ifndef USART_H
#define USART_H

#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>

//������ ������
#define SIZE_BUF	64


void USART_Init(unsigned int baudrate); //������������� usart`a
unsigned char USART_GetTxCount(void); //����� ����� �������� ����������� ������
void USART_FlushTxBuf(void); //�������� ���������� �����
void USART_PutChar(unsigned char sym); //�������� ������ � �����
void USART_SendStr(unsigned char * data); //������� ������ �� usart`�
void USART_SendArray(unsigned char *data, uint8_t size)	;
unsigned char USART_GetRxCount(void); //����� ����� �������� � �������� ������
void USART_FlushRxBuf(void); //�������� �������� �����
unsigned char USART_GetChar(void); //��������� �������� ����� usart`a 


#endif //USART_H