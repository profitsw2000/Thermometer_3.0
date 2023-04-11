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
#include "usart.h"

//���������� �����
unsigned char usartTxBuf[SIZE_BUF];
unsigned char txBufTail = 0;
unsigned char txBufHead = 0;
unsigned char txCount = 0;

//�������� �����
unsigned char usartRxBuf[SIZE_BUF];
unsigned char rxBufTail = 0;
unsigned char rxBufHead = 0;
unsigned char rxCount = 0;

//������������� usart`a
void USART_Init(unsigned int baudrate)
{
  unsigned long ubr_reg = ((F_CPU/16)/(baudrate)) - 1	;
  
  UBRRH = ubr_reg/256	;
  UBRRL = ubr_reg%256	;
  UCSRB = (1<<RXCIE)|(1<<TXCIE)|(1<<RXEN)|(1<<TXEN); //����. ������ ��� ������ � ��������, ���� ������, ���� ��������.
  UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0); //������ ����� 8 ��������
}

//______________________________________________________________________________
//���������� ����������� �������� ����������� ������
unsigned char USART_GetTxCount(void)
{
  return txCount;  
}

//"�������" ���������� �����
void USART_FlushTxBuf(void)
{
  txBufTail = 0;
  txCount = 0;
  txBufHead = 0;
}

//�������� ������ � �����, ���������� ������ ��������
void USART_PutChar(unsigned char sym)
{
  //���� ������ usart �������� � ��� ������ ������
  //����� ��� ����� � ������� UDR
  if(((UCSRA & (1<<UDRE)) != 0) && (txCount == 0)) UDR = sym;
  else {
    if (txCount < SIZE_BUF){    //���� � ������ ��� ���� �����
      usartTxBuf[txBufTail] = sym; //�������� � ���� ������
      txCount++;                   //�������������� ������� ��������
      txBufTail++;                 //� ������ ������ ������
      if (txBufTail == SIZE_BUF) txBufTail = 0;
    }
  }
}

//������� ���������� ������ �� usart`�
void USART_SendStr(unsigned char *data)
{
  unsigned char sym;
  while(*data){
    sym = *data++;
    USART_PutChar(sym);
  }
}

//������� ���������� ������ ������� size �� usart`�
//produced by profitsw2000
void USART_SendArray(unsigned char *data, uint8_t size)
{
	unsigned char sym;
	
	if (size > SIZE_BUF) return	;
	for (uint8_t i = 0; i<size; i++)
	{
		sym = *data++;
		USART_PutChar(sym);
	}
}

//���������� ���������� �� ���������� �������� 
ISR(USART_TXC_vect)
{
  if (txCount > 0){              //���� ����� �� ������
    UDR = usartTxBuf[txBufHead]; //���������� � UDR ������ �� ������
    txCount--;                   //��������� ������� ��������
    txBufHead++;                 //�������������� ������ ������ ������
    if (txBufHead == SIZE_BUF) txBufHead = 0;
  } 
} 

//______________________________________________________________________________
//���������� ����������� �������� ����������� � �������� ������
unsigned char USART_GetRxCount(void)
{
  return rxCount;  
}

//"�������" �������� �����
void USART_FlushRxBuf(void)
{
  unsigned char saveSreg = SREG;
  cli();
  rxBufTail = 0;
  rxBufHead = 0;
  rxCount = 0;
  SREG = saveSreg;
}

//������ ������
unsigned char USART_GetChar(void)
{
  unsigned char sym;
  if (rxCount > 0){                     //���� �������� ����� �� ������  
    sym = usartRxBuf[rxBufHead];        //��������� �� ���� ������    
    rxCount--;                          //��������� ������� ��������
    rxBufHead++;                        //���������������� ������ ������ ������  
    if (rxBufHead == SIZE_BUF) rxBufHead = 0;
    return sym;                         //������� ����������� ������
  }
  return 0;
}


//���������� �� ���������� ������
ISR(USART_RXC_vect) 
{
  if (rxCount < SIZE_BUF){                //���� � ������ ��� ���� �����                     
      usartRxBuf[rxBufTail] = UDR;        //������� ������ �� UDR � �����
      rxBufTail++;                             //��������� ������ ������ ��������� ������ 
      if (rxBufTail == SIZE_BUF) rxBufTail = 0;  
      rxCount++;                                 //��������� ������� �������� ��������
    }
} 

