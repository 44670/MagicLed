#include "global.h"
#include "uart.h"
#include <stdio.h>

#define SERIAL_INTERRUPT 1
u8 dbuff[20];

unsigned char UART_lastbyte = 0;

void UartInit(void)		//9600bps@11.0592MHz
{
	SCON = 0x50;		
	AUXR |= 0x40;		
	AUXR &= 0xFE;		
	TMOD &= 0x0F;		
	TL1 = 0xE0;		
	TH1 = 0xFE;		
	ET1 = 0;		
	TR1 = 1;		
	
	ES = SERIAL_INTERRUPT;
}



void UART_txdata(unsigned char dat)
{
#if DEBUG == 1
	return;
#endif
	ES = 0;
  SBUF = dat; //发送数据
  while (!TI)
    ;
  //等待数据发送完中断
  TI = 0; //清中断标志  
	ES = SERIAL_INTERRUPT;
}

unsigned char UART_rxdata()
{
  unsigned char dat;
	ES = 0;
  while (!RI)
    ;
  //等待数据接收完
  dat = SBUF; //接收数据
  RI = 0; //清中断标志
	ES = SERIAL_INTERRUPT;
  return (dat);
}

void UART_sendstr(unsigned char str[])
{
  unsigned char i = 0;

  while (str[i] != '\0')
  {
    UART_txdata(str[i++]);
  }
}

void d_wrint(unsigned char str[],int d)
{
	UART_sendstr(str);
	sprintf(dbuff,"%u,",d);
	UART_sendstr(dbuff);
}

void reset() {
			 EA = 0;
			delayms(2000);
		 IAP_CONTR = 0x60;while(1);
}


