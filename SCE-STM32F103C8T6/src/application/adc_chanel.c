#include "global.h"
#include "core_cm3.h"

#define SAMPLES_NUM 64
#define SAMPLES_SHIFT 6
volatile uint16_t _overBuffer[SAMPLES_NUM], _chanel_value;

static void adc_start() {
	ADC_DeInit(ADC1);

	DMA_InitTypeDef DMA_InitStructure;
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(((ADC_TypeDef*)ADC1_BASE)->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)_overBuffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = SAMPLES_NUM;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16 bit
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; //16 bit
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1, ENABLE);
	/* Enable DMA Channel6 Transfer Complete interrupt */
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

 ADC_InitTypeDef ADC_InitStructure;
 ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
 ADC_InitStructure.ADC_ScanConvMode = ENABLE;
 ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
 ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
 ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
 ADC_InitStructure.ADC_NbrOfChannel = 1;
 ADC_Init(ADC1, &ADC_InitStructure);
 ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_239Cycles5);
 ADC_DMACmd(ADC1, ENABLE);
 ADC_Cmd(ADC1, ENABLE);
 ADC_ResetCalibration(ADC1);
 while(ADC_GetResetCalibrationStatus(ADC1));
 ADC_StartCalibration(ADC1);
 while(ADC_GetCalibrationStatus(ADC1));
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void adc_DMA1_Chanel1IRQ() {
 if(DMA_GetITStatus(DMA1_IT_TC1)) {
   /* Get Current Data Counter value after complete transfer */
   /* Clear DMA Channel1 Half Transfer, Transfer Complete and Global interrupt pending bits */
  DMA_ClearITPendingBit(DMA1_IT_GL1);
  uint32_t sum = 0;
  for(int i = 0; i < SAMPLES_NUM; i++) sum += _overBuffer[i];
  sum = sum >> SAMPLES_SHIFT;
  _chanel_value = (uint16_t)sum;
  adc_start();
 }
}

void adc_init() {
	// ADC:  PA2
	_chanel_value = 0;
 GPIO_InitTypeDef GPIO_InitStructure;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
 GPIO_Init(GPIOA, &GPIO_InitStructure);

 NVIC_InitTypeDef NVIC_InitStructure;
 NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
 NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
 NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
 NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
 NVIC_Init(&NVIC_InitStructure);
 adc_start();
}

uint16_t adc_getValue() {
	return _chanel_value;
}
