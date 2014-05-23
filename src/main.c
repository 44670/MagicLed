#include "global.h"
#include "UART.h"
#include <stdlib.h>
#include <string.h>

sbit SCK= P1^5 ;
sbit SDI= P1^3;
sbit LE= P2^1;
sbit OE= P2^2;
sbit BTN = P2^4;
sbit LED = P2^3;
sbit IR_T = P1^1;
sbit IR_R = P3^3;

u8 xdata spiBuff[8][8][3] ;
u8 xdata pixBuff[8][24]; 
u16 xdata magic;
u8 curRow;
u8 curScan;
u8 aniMode;
u8 ppixBuff;
u8 needUpdate;

void Delay1ms()		//@11.0592MHz
{
	unsigned char i, j;

	_nop_();
	_nop_();
	_nop_();
	i = 11;
	j = 190;
	do
	{
		while (--j);
	} while (--i);
}

void delayms(unsigned int a) {
	unsigned int i;
	for (i=0;i<a;i++) Delay1ms();
}


void Delay5us()		//@11.0592MHz
{
	unsigned char i;

	_nop_();
	i = 11;
	while (--i);
}

void spiInit()
{
	SPDAT = 0;                  
	SPSTAT = SPIF | WCOL;       
	SPCTL = SPEN | MSTR;       
}

void spiSend(u8 dat) {
#if DEBUG == 1
	return;
#endif
	SPDAT = dat;                
	while (!(SPSTAT & SPIF));   
	SPSTAT = SPIF | WCOL;       
}

void Timer0Init(void)		//100us@11.0592MHz
{
	AUXR |= 0x80;		
	TMOD &= 0xF0;		
	TL0 = 0xAE;		
	TH0 = 0xFB;
	TF0 = 0;		
	TR0 = 1;		
}

/* Sacn one line */
void onTimer0Interrupt(void) interrupt 1 using 1 {
	u8 xdata *ptr;

	EA = 0;
	TR0 = 0;
	curScan++;
	if (curScan >= 8) {
		curScan = 0;
		curRow++;
		if (curRow >= 8) curRow = 0;
		OE = 1;
		P0 = ~(1 << curRow);	
	}
	LE = 0;
	ptr = &spiBuff[curRow][curScan][0];
	spiSend(*ptr);
	spiSend(*(++ptr));
	spiSend(*(++ptr));
	LE = 1;
	OE = 0;
	TR0 = 1;
	EA = 1;
}

/* Generate buffer of one line */
void getScanCode(u8* buf, u8* pix) {
	u8 i, j;
	memset(buf, 0, 8);
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8;j++) {
			if (pix[i] > j) {
				buf[j] |= (1 << i);
			}
		}
	}
}

/* Generate buffer from pixel data */
void updateSpiBuff() {
	u8 i, j, k;
	u8 xdata buf[8];
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 3; j++) {
			getScanCode(buf, &pixBuff[i][j * 8]);
			for (k = 0; k < 8; k++) {
				spiBuff[i][k][j] = buf[k];
			}
		}
	}
}

/* Draw the next frame of animation. */
void playAni() {
	char i,j,t;
	static char s[3], d[3];
	if (aniMode == 0) return;
	if (aniMode > 2) return;
	if (aniMode == 1) {
		for (i = 0; i < 8; i++) 
			for (j = 0; j < 8; j++) {
				pixBuff[i][j] = rand() % 8;
				pixBuff[i][j+16] = rand() % 8;
				pixBuff[i][j+8] = rand() % 8;
			}
	}
	if (aniMode == 2) {
		for (i = 0; i < 3; i++) {
			if ((s[i] < 0) || (s[i] >=8)) {
				s[i] = 0;
			}
			if (d[i] != -1 && d[i] != 1) d[i] = (rand() % 2) ?  1 : -1;
			t = s[i] + d[i];
			if ((t < 0) || (t >= 8)) {
				if ((rand() % 10) < 3) {
					t = s[i];
				} else {
					d[i] = -d[i];
					t = s[i] + d[i];
				}
			}
			s[i] = t;
		}
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				pixBuff[i][j] = (i > s[1]) ? (7-(i-s[1])) : (7-(s[1]-i));
				pixBuff[i][j+8] = (j > s[2]) ? (7-(j-s[2])) : (7-(s[2]-j));
				pixBuff[i][j+16] = (i > s[0]) ? (7-(i-s[0])) : (7-(s[0]-i));
			}
		}
	}
	updateSpiBuff();

}

/* Hardware test. */
void test() {
	u8 i, j;
	while(1) {
		memset(pixBuff, 0, sizeof(pixBuff));
		updateSpiBuff();
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 24; j++) {
				pixBuff[i][j] = 1;
				updateSpiBuff();
				delayms(1000);
			}
		}
		delayms(1000);
	}
}


void MainLoop() {
	int i, j;
	u8 r,g,b,t;


	aniMode = 0;
	ppixBuff = 0;
	needUpdate = 0;

	//test();


	while(1) {
		for (i = 0; i < 100; i++) {
			Delay1ms();
			if (!BTN) break;
			if (needUpdate) break;			
		}
		if (needUpdate) {
			needUpdate = 0;
			updateSpiBuff();
		}
		if (!BTN) {
			delayms(10);
			if (!BTN) {
				while(1) {
					if (BTN) {
						delayms(10);
						if (BTN) break;
					}
				}
				aniMode ++;
				if (aniMode > 2) {			
					if (aniMode > 9) {
						aniMode = 0;
						memset(pixBuff, 0, sizeof(pixBuff));
						updateSpiBuff();
						continue;
					}

					t = aniMode - 2;
					r = t / 4;
					g = (t / 2) % 2;
					b = t % 2;
					r = r * 7; g = g * 7; b = b * 7;
					for (i = 0; i < 8; i++) 
						for (j = 0; j < 8; j++) {
							pixBuff[i][j] = r;
							pixBuff[i][j+8] = b;
							pixBuff[i][j+16] = g;
						}
						updateSpiBuff();
				}
			}
		}
		playAni();
	}
}

/* Process UART commands. */
void onUARTInterrupt() interrupt 4
{
	unsigned char ch;
	EA = 0;
	/* Ignore transmit events. */
	if (TI)
	{
		TI = 0;
		goto final;
	}

	if (RI)
	{
		RI = 0;
		ch = SBUF;
		/* Hardware reset (for ISP flashers) */
		if (ch == 0xfc) {
			reset();
			while(1);
		}
		/* Pixel data */
		if ((ch & 0x80) == 0) {
			/* Check buffer size */
			if (ppixBuff >= sizeof(pixBuff)) goto final;   
			pixBuff[0][ppixBuff] = (ch & 0x7);
			pixBuff[0][ppixBuff+1] = (ch & 0x38) >> 3;
			ppixBuff += 2;
			goto final;
		}
		switch(ch) {
		/* Clear pixel buffer */
		case 0xfa:
			memset(pixBuff, 0, sizeof(pixBuff));
		/* Update pixel buffer to LED */
		case 0xfb:
			aniMode = 0;
			ppixBuff = 0;
			needUpdate = 1;
			break;
		/* Play animations */
		case 0xfd:
			aniMode = 1;
			break;
		case 0xfe:
			aniMode = 2;
			break;
		case 0xff:
			aniMode = 3;
			break;		
		}
	}
	final:

	EA = 1;
}

void main() {
	int i,j,t, k;

	delayms(100);
	UartInit();
	EA = 1;
	UART_sendstr("hello");
	spiInit();
	spiSend(0);
	spiSend(0);
	spiSend(0);
	curRow = 0;
	curScan = 0;
	memset(spiBuff, 0, sizeof(spiBuff));
	memset(pixBuff, 0, sizeof(pixBuff));
	LE = 1;
	OE = 1;
	P0 = 0xff;
	//P0M0 = 0xff;
	Timer0Init();
	ET0 = 1;
	MainLoop();
	while(1);

}