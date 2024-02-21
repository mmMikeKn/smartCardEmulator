#include <string.h>
#include <stdarg.h>

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <core_cm3.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_rtc.h>
#include "com_rf_io.h"
#include "global.h"

static volatile unsigned char rf_TxBuffer[512];
static volatile unsigned short rf_TxBufferSz = 0, rf_TxBufferPtr = 0;
static volatile unsigned char isTrnsmitEnd = TRUE;

void	rf_putChar() {
	USART_SendData(USART1, (uint16_t)rf_TxBuffer[rf_TxBufferPtr++]);
	if(rf_TxBufferPtr >= rf_TxBufferSz) {
	 USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	 isTrnsmitEnd = TRUE;
	}
}

#define STX 0x02

unsigned char rf_isTxEmpty() {
	return isTrnsmitEnd;
}

// STX[2] || code[2] || time[2] || sz[2] || body[sz] || lrc

static void putCmd(short code, const uint8_t *data, int sz) {
 while(!rf_isTxEmpty()) __NOP();

 rf_TxBuffer[0] = STX;
 rf_TxBuffer[1] = (unsigned char)(code >> 8); rf_TxBuffer[2] = (unsigned char)code;
 rf_TxBuffer[3] = (unsigned char)(_sysTicks >> 8); rf_TxBuffer[4] = (unsigned char)_sysTicks;
 rf_TxBuffer[5] = (unsigned char)(sz >> 8);
 rf_TxBuffer[6] = (unsigned char)sz;
 memcpy((unsigned char*)rf_TxBuffer+7, data, sz); rf_TxBuffer[7+sz] = 0;
 for(int i = 0; i < (sz+6); i++) rf_TxBuffer[7+sz] ^= rf_TxBuffer[1+i];
 rf_TxBufferSz = sz+8, rf_TxBufferPtr = 0; isTrnsmitEnd = FALSE;
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	while(!rf_isTxEmpty()) {}
}

static unsigned char rf_RxBuffer[512];
static volatile unsigned short rf_RxBufferPtr = 0;
static volatile unsigned int rf_RxDataSz = 0;
static volatile short rf_cmd = RF_NO_CMD;

// STX || code[2] || sz[2] || body[sz] || lrc
void rf_procChar(unsigned char c) {
	if(rf_RxBufferPtr == 0 && c != STX) return;
	rf_RxBuffer[rf_RxBufferPtr++] = c;

	if(rf_RxBufferPtr == 5) {
		rf_RxDataSz = ((unsigned int)rf_RxBuffer[3] << 8) | (unsigned int)rf_RxBuffer[4];
	}
	if(rf_RxDataSz > (sizeof(rf_RxBuffer) - 2-1)) { rf_RxBufferPtr = 0; return; }
 int crcOfs = rf_RxDataSz + 5;
	if(rf_RxBufferPtr > crcOfs) {
		for(int i = 1; i < crcOfs; i++) rf_RxBuffer[crcOfs] ^= rf_RxBuffer[i];
 	if(rf_RxBuffer[crcOfs] != 0) rf_cmd = RF_WRONG_DATA;
		else {
		 rf_cmd = ((unsigned short)rf_RxBuffer[1] << 8) | (unsigned short)rf_RxBuffer[2];
		 USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
		}
		rf_RxBufferPtr = 0;
	}
}

void rf_sendDataNoWait(short code, unsigned char *data, unsigned char  sz) {
	putCmd(code, data, sz);
}


short rf_getCmdCode(void) {
	return rf_cmd;
}

unsigned char rf_getCmdDump(unsigned char *data) {
	memcpy(data, rf_RxBuffer, rf_RxDataSz+5);
	return rf_RxDataSz+5;
}

uint8_t rf_getCmdBody(uint8_t *data, int maxSz) {
	uint8_t sz = rf_RxDataSz;
	memcpy(data, rf_RxBuffer+5, maxSz < sz? maxSz:sz);	rf_cmd = RF_NO_CMD;
 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
 return sz;
}

void rf_init() {
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
 GPIO_InitTypeDef GPIO_InitStructure;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

 NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitTypeDef NVIC_InitStructure;
 NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
 GPIO_Init(GPIOA, &GPIO_InitStructure);
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
 GPIO_Init(GPIOA, &GPIO_InitStructure);

 USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &USART_InitStructure);
	USART_Cmd(USART1, ENABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
}

// ---------------------------
static unsigned char rf_itoa(long val, int radix, int len, char *sout, unsigned char ptr) {
	unsigned char c, r, sgn = 0, pad = ' ';
	unsigned char s[20], i = 0;
	unsigned long v;

	if (radix < 0) {
		radix = -radix;
		if (val < 0) {		val = -val;	sgn = '-';	}
	}
	v = val;
	r = radix;
	if (len < 0) {	len = -len;	pad = '0'; }
	if (len > 20) return ptr;
	do {
		c = (unsigned char)(v % r);
		if (c >= 10) c += 7;
		c += '0';
		s[i++] = c;
		v /= r;
	} while (v);
	if (sgn) s[i++] = sgn;
	while (i < len)	s[i++] = pad;
	do	sout[ptr++] = (s[--i]);
	while (i);
	return ptr;
}

void cmd_sendMsg(const char* str, ...) {
	va_list arp;
	int d, r, w, s, l;
	va_start(arp, str);
	char sout[256];
	unsigned char ptr = 0;

	while ((d = *str++) != 0) {
			if (d != '%') {	sout[ptr++]=d; continue;	}
			d = *str++; w = r = s = l = 0;
			if (d == '0') {
				d = *str++; s = 1;
			}
			while ((d >= '0')&&(d <= '9')) {
				w += w * 10 + (d - '0');
				d = *str++;
			}
			if (s) w = -w;
			if (d == 'l') {
				l = 1;
				d = *str++;
			}
			if (!d) break;
			if (d == 's') {
				char *s = va_arg(arp, char*);
				while(*s != 0) { sout[ptr++] = *s; s++; }
				continue;
			}
			if (d == 'c') {
				sout[ptr++] = (char)va_arg(arp, int);
				continue;
			}
			if (d == 'u') r = 10;
			if (d == 'd') r = -10;
			if (d == 'X' || d == 'x') r = 16; // 'x' added by mthomas in increase compatibility
			if (d == 'b') r = 2;
			if (!r) break;
			if (l) {
				ptr = rf_itoa((long)va_arg(arp, long), r, w, sout, ptr);
			} else {
				if (r > 0) ptr = rf_itoa((unsigned long)va_arg(arp, int), r, w, sout, ptr);
				else	ptr = rf_itoa((long)va_arg(arp, int), r, w, sout, ptr);
			}
	}
	va_end(arp);
	sout[ptr] = 0;
 putCmd(CMD_SHOW_TEXT, (uint8_t*)sout, strlen(sout)+1);
}

void cmd_ATR_RQ() {
	uint8_t data[1];
	data[0] = 0;
	putCmd(CMD_ATR_RQ, data, 1);
}

void cmd_APDU_IFD_T0(const uint8_t *data, int sz) {
	putCmd(CMD_APDU_IFD_T0, data, sz);
}

void cmd_APDU_BODY_IFD_T0(const uint8_t *data, int sz) {
	putCmd(CMD_APDU_BODY_IFD_T0, data, sz);
}

void cmd_eventATR(uint8_t isWarm) {
	uint8_t data[1];
	data[0] = isWarm;
	putCmd(CMD_ATR_EVENT, data, 1);
}

void cmd_IFD_T1(const uint8_t *data, int sz) {
	putCmd(CMD_IFD_T1, data, sz);
}

void cmd_ICC_T1(const uint8_t *data, int sz) {
	putCmd(CMD_ICC_T1, data, sz);
}

void cmd_dbg_dump(const uint8_t *data, int sz) {
	putCmd(CMD_DBG_DUMP, data, sz);
}
