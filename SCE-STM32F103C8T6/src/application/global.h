/*
 * global.h
 *
 *  Created on: 14.04.2011
 *      Author: mm
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_rtc.h"

#include "hw_config.h"
#include "com_rf_io.h"

extern volatile uint32_t _sysTicks;
void delayMs(uint32_t msec);
void delayUs(uint32_t usec);

// ADC: PA2- card VCC
// PB6(TIM4_CH1) - CLK
// PB8 - RST
// PB7 - I/O

#define CLK_PIN GPIO_Pin_6
#define CLK_PORT GPIOB
#define IO_PIN GPIO_Pin_7
#define IO_PORT GPIOB
#define RST_PIN GPIO_Pin_8
#define RST_PORT GPIOB

#define DBG_PIN GPIO_Pin_9
#define DBG_PORT GPIOB
#define DBG_LCD_PIN GPIO_Pin_13
#define DBG_LED_PORT GPIOC

void adc_DMA1_Chanel1IRQ();
uint16_t adc_getValue();
void adc_init();

#define USE_VCC_3V

inline uint8_t chkVcc() {
	int16_t vcc = adc_getValue();
#ifdef VCC_3V
	if(vcc < 1500 || vcc > 2500) { // 1.8V < ~3V
#else
		if(vcc < 2500) { //  < 5V
#endif
	 return FALSE;
	}
	return TRUE;
}

#define  MAX_ATR_SIZE   33
extern uint8_t atr[MAX_ATR_SIZE];
extern uint8_t atr_sz;

#define SCARD_PROTOCOL_T0   0x0001
#define SCARD_PROTOCOL_T1   0x0002
#define SCARD_PROTOCOL_UNDEFINED   0x0000

#define ST_IO_OUT_OK (0)
#define ST_IO_OUT_TIMEOUT (-1)
#define ST_IO_OUT_LOW (-2)
#define ST_IO_OUT_PARITY_ERR (-3)
#define ST_IO_INP_PARITY_ERR (-4)
#define ST_IO_INP_WR0NG_START_BIT (-5)
#define ST_IO_INP_TIMEOUT (-6)
#define ST_IO_INP_WRONG_STOP_BIT (-7)
#define ST_IO_VCC_OFF (-10)
#define ST_IO_SHUTDOWN (-11)
#define ST_IO_RESET_OFFSET (-100)
#define ST_IO_WARM_RESET_OFFSET (-1000)

void cardIO_ClkInit();

int cardIO_waitRvc(uint8_t *buf, int sz, uint32_t timeout);
int cardIO_sendByte(uint8_t b);
int cardIO_sendData(uint8_t *data, uint8_t sz);
int cardIO_RESET(uint8_t *atr, uint8_t atr_sz);

void smartCardLifeT0();
void smartCardLifeT1();

#endif /* GLOBAL_H_ */
