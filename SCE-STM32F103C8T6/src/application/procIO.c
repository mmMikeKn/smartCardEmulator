#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "com_rf_io.h"

// PB7 - I/O
inline void _card_IO_inpMode() {
 GPIO_InitTypeDef GPIO_InitStructure;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
 GPIO_InitStructure.GPIO_Pin = IO_PIN; GPIO_Init(IO_PORT, &GPIO_InitStructure);
}

inline void _card_IO_outpMode() {
 GPIO_InitTypeDef GPIO_InitStructure;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
 GPIO_InitStructure.GPIO_Pin = IO_PIN; GPIO_Init(IO_PORT, &GPIO_InitStructure);
}

//=========================================================================
#define MAX_REPEAT_CNT 0

inline uint8_t parity_even_bit(uint8_t x){
 x ^= x >> 8;
 x ^= x >> 4;
 x ^= x >> 2;
 x ^= x >> 1;
 return x & 1;
}

static inline void startTimer() {
	TIM_Cmd(TIM4, DISABLE);
	TIM4->CNT = 0;
 TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_Cmd(TIM4, ENABLE);
}

static inline uint8_t etu_half_wait() {
 uint32_t t = _sysTicks;
 while(_sysTicks - t < 10) {
  if(TIM_GetFlagStatus(TIM4, TIM_IT_Update) == SET) {
  	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
  	return TRUE;
  }
 }
 return FALSE;
}

int cardIO_sendByte(uint8_t b) {
 uint8_t parityBit = parity_even_bit(b);
 // check IO state
 _card_IO_inpMode();
 if(!(IO_PORT->IDR & IO_PIN)) return ST_IO_OUT_LOW;
 _card_IO_outpMode();
 // start bit
 IO_PORT->BRR = IO_PIN;
 startTimer();
 if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 for(int i = 0, bit = 0x01; i < 8; i++, bit = bit << 1) {
 	if((b & bit) != 0) IO_PORT->BSRR = IO_PIN;
 	else IO_PORT->BRR = IO_PIN;
  if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
  if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 }
 // parity bits
	if(parityBit) IO_PORT->BSRR = IO_PIN;
	else IO_PORT->BRR = IO_PIN;
 if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 // stop bits
 IO_PORT->BSRR = IO_PIN;
 if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 _card_IO_inpMode();
 if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 if(!(IO_PORT->IDR & IO_PIN)) return ST_IO_OUT_PARITY_ERR;
 if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 if(!(IO_PORT->IDR & IO_PIN)) return ST_IO_OUT_PARITY_ERR;
 if(!etu_half_wait()) return ST_IO_OUT_TIMEOUT;
 return ST_IO_OUT_OK;
}

int cardIO_recvByte(uint32_t timeout) {
	uint8_t b = 0, parityBit;
 _card_IO_inpMode();
 uint32_t t = _sysTicks;
 while((IO_PORT->IDR & IO_PIN) != 0) { //waiting start bit
 	if(_sysTicks - t > timeout) return ST_IO_INP_TIMEOUT;
 }
 startTimer();
 // start bit
 if(!etu_half_wait()) return ST_IO_INP_TIMEOUT;
 if((IO_PORT->IDR & IO_PIN) != 0) return ST_IO_INP_WR0NG_START_BIT;
 if(!etu_half_wait()) return ST_IO_INP_TIMEOUT;
 for(int i = 0, bit = 1; i < 8; i++, bit = bit << 1) {
  if(!etu_half_wait()) return ST_IO_INP_TIMEOUT;
  if((IO_PORT->IDR & IO_PIN) != 0) b |= bit;
  if(!etu_half_wait()) return ST_IO_INP_TIMEOUT;
 }
 // parity bits
 if(!etu_half_wait()) return ST_IO_INP_TIMEOUT;
 parityBit = (IO_PORT->IDR & IO_PIN) != 0;
 if(!etu_half_wait()) return ST_IO_INP_TIMEOUT;
 if(parityBit != parity_even_bit(b)) return ST_IO_INP_PARITY_ERR;

 // stop bit.. 0.5 etu
 if(!etu_half_wait()) return ST_IO_INP_TIMEOUT;
 if((IO_PORT->IDR & IO_PIN) == 0) return ST_IO_INP_WRONG_STOP_BIT;
 return b;
}

//------------------------------
int cardIO_waitRvc(uint8_t *buf, int sz, uint32_t timeout) {
	while(TRUE) {
		if((IO_PORT->IDR & IO_PIN) == 0) break; // start bit
	 if(!chkVcc()) return ST_IO_VCC_OFF;
  if((RST_PORT->IDR & RST_PIN) == 0) { // RESET
   while((RST_PORT->IDR & RST_PIN) == 0) {
   	if(!chkVcc()) return ST_IO_SHUTDOWN;
   }
  	int st = cardIO_RESET(atr, atr_sz);
   if(st != 0) return st+ST_IO_WARM_RESET_OFFSET;
   cmd_eventATR(TRUE);
  }
	}

	for(int i = 0; i < sz; i++) {
		int st = cardIO_recvByte(timeout);
		if(st < 0) return st;
		buf[i] = (uint8_t)st;
	}
	return 0;
}

int cardIO_sendData(uint8_t *data, uint8_t sz) {
 for(int i = 0; i < sz; i++) {
 	int st = cardIO_sendByte(data[i]);
 	if(st < 0) return st;
 }
 return ST_IO_OUT_OK;
}

int cardIO_RESET(uint8_t *atr, uint8_t atr_sz) {
 delayUs(300);
 int st = cardIO_sendByte(atr[0]); // TS
 if(st < 0) return st+ST_IO_RESET_OFFSET;
 delayUs(300); // 12 etu <= t <= 9600 etu
 st = cardIO_sendData(atr+1, atr_sz-1);
 if(st < 0) return st+ST_IO_RESET_OFFSET;
 return ST_IO_OUT_OK;
}

void cardIO_ClkInit() {
 RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_Cmd(TIM4, DISABLE);
 GPIO_InitTypeDef GPIO_InitStructure;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
 GPIO_InitStructure.GPIO_Pin = CLK_PIN; GPIO_Init(CLK_PORT, &GPIO_InitStructure);
//
 TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
 TIM_TimeBaseStructure.TIM_Prescaler = 0;
 TIM_TimeBaseStructure.TIM_Period = (372/2)+1; // Fd = 372 and Dd = 1
 TIM_TimeBaseStructure.TIM_ClockDivision = 0;
 TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
 TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x0;
 TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
 TIM_ARRPreloadConfig(TIM4, ENABLE);
 TIM_TIxExternalClockConfig(TIM4, TIM_TIxExternalCLK1Source_TI1, TIM_ICPolarity_Rising, 1);
}
