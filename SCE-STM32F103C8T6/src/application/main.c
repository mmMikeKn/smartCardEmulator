#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "com_rf_io.h"

uint8_t atr[MAX_ATR_SIZE];
uint8_t atr_sz;
static uint8_t cardProtocol;


void delayMs(uint32_t msec)  {
 uint32_t tmp = 7000 * msec  ;
 while( tmp-- ) __NOP();
}

inline void delayUs(uint32_t usec)  {
 uint32_t tmp = 7 * usec  ;
 while( tmp-- ) __NOP();
}


void parseATR(uint8_t *atr, uint8_t atr_sz) {
	uint8_t Y1i, T;
	uint16_t p = 2;
 int i = 1;

 cardProtocol = SCARD_PROTOCOL_UNDEFINED;
 if(atr_sz < 2 && ((atr[0] != 0x3F) && (atr[0] != 0x3B))) return;
 Y1i = atr[1] >> 4;
 do  {
 	short TAi, TBi, TCi, TDi;
 	TAi = (Y1i & 0x01) ? atr[p++] : -1;
 	TBi = (Y1i & 0x02) ? atr[p++] : -1;
 	TCi = (Y1i & 0x04) ? atr[p++] : -1;
 	TDi = (Y1i & 0x08) ? atr[p++] : -1;
 	(void)TBi;
 	(void)TCi;
 	if (TDi >= 0) {
 		Y1i = TDi >> 4;
 		T = TDi & 0x0F;
 		if(T == 0) cardProtocol = SCARD_PROTOCOL_T0;
 		else if(T == 1) cardProtocol = SCARD_PROTOCOL_T1;
 		else return;
 	} else {
 		Y1i = 0;
 	}
 	if ((2 == i) && (TAi >= 0)) { //test presence of TA2
 		T = TAi & 0x0F;
 		if(T == 0) cardProtocol = SCARD_PROTOCOL_T0;
 		else if(T == 1) cardProtocol = SCARD_PROTOCOL_T1;
 		else return;
 	}
  if(p > MAX_ATR_SIZE) return;
 	i++;
 } while (Y1i != 0);
}

#define TEST_T0

int main() {
 SystemStartup();
 rf_init();
 adc_init();
 cardIO_ClkInit();

 DBG_LED_PORT->BSRR = DBG_LCD_PIN;
 DBG_PORT->BSRR = DBG_PIN;

#ifdef TEST_T0
 uint8_t ATR_T0[] = {0x3B,0x6D,0x00,0x00,0x80,0x31,0x80,0x65,0xB0,0x89,0x40,0x01,0xF2,0x83,0x00,0x90,0x00};
 atr_sz = sizeof(ATR_T0);
 memcpy(atr, ATR_T0, atr_sz);
#else
 uint8_t ATR_T1[] = {0x3B,0xEA,0x00,0x00,0x81,0x31,0xFE,0x45,0x00,0x31,0xC1,0x73,0xC8,0x40,0x00,0x00,0x90,0x00,0x7A};
 atr_sz = sizeof(ATR_T1);
 memcpy(atr, ATR_T1, atr_sz);
#endif

 cmd_ATR_RQ();
 uint32_t t = _sysTicks;
 while((_sysTicks-t) < 2000) {
 	if(rf_getCmdCode() == CMD_ATR_DATA) {
 		atr_sz = rf_getCmdBody(atr, sizeof(atr));
 		break;
 	}
 }

 parseATR(atr, atr_sz);
//	cmd_sendMsg("ATR T=%d (%d)", cardProtocol == SCARD_PROTOCOL_T1? 1:0, atr_sz);

	while(TRUE) {
		while((RST_PORT->IDR & RST_PIN) == 0 || !chkVcc()) {
			// idle state
		}
	 DBG_LED_PORT->BRR = DBG_LCD_PIN;
		cardIO_RESET(atr, atr_sz); // cold reset
		cmd_eventATR(FALSE);
		if(cardProtocol == SCARD_PROTOCOL_T1) smartCardLifeT1();
		else smartCardLifeT0();
	 DBG_LED_PORT->BSRR = DBG_LCD_PIN;
		while((RST_PORT->IDR & RST_PIN) != 0) {};
	}
}


