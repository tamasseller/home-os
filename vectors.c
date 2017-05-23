#include <stdint.h>

__attribute__((naked))
void DefaultHandler(void) {
    for(;;){}
}

__attribute__((naked))
void DefaultHardFaultHandler(void){
    for(;;){}
}

void _estack(void);
void Reset_Handler(void) __attribute__((weak, alias("DefaultHandler")));
void NMI_Handler(void) __attribute__((weak, alias("DefaultHandler")));
void HardFault_Handler(void) __attribute__((weak, alias("DefaultHardFaultHandler")));
void SVC_Handler(void) __attribute__((weak, alias("DefaultHandler")));
void PendSV_Handler(void) __attribute__((weak, alias("DefaultHandler")));
void SysTick_Handler(void) __attribute__((weak, alias("DefaultHandler")));

void WWDG_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void PVD_VDDIO2_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void RTC_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void FLASH_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void RCC_CRS_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void EXTI0_1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void EXTI2_3_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void EXTI4_15_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TSC_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void DMA1_Channel1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void DMA1_Channel2_3_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void DMA1_Channel4_5_6_7_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void ADC1_COMP_IRQHandler (void) __attribute__((weak, alias("DefaultHandler")));
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TIM1_CC_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TIM3_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TIM6_DAC_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TIM7_IRQHandler    (void) __attribute__((weak, alias("DefaultHandler")));
void TIM14_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TIM15_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TIM16_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void TIM17_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void I2C1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void I2C2_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void SPI2_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void USART3_4_IRQHandler (void) __attribute__((weak, alias("DefaultHandler")));
void CEC_CAN_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void USB_IRQHandler(void) __attribute__((weak, alias("DefaultHandler")));
void BootRAM    (void) __attribute__((weak, alias("DefaultHandler")));

typedef void* funcp;

__attribute__((section(".isr_vector")))
funcp vector_table[] = {
	(void*)_estack, /* &_estack*/
	Reset_Handler,
	NMI_Handler,
	HardFault_Handler,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	SVC_Handler,
	0,
	0,
	PendSV_Handler,
	SysTick_Handler,
	WWDG_IRQHandler,
	PVD_VDDIO2_IRQHandler,
	RTC_IRQHandler,
	FLASH_IRQHandler,
	RCC_CRS_IRQHandler,
	EXTI0_1_IRQHandler,
	EXTI2_3_IRQHandler,
	EXTI4_15_IRQHandler,
	TSC_IRQHandler,
	DMA1_Channel1_IRQHandler,
	DMA1_Channel2_3_IRQHandler,
	DMA1_Channel4_5_6_7_IRQHandler,
	ADC1_COMP_IRQHandler,
	TIM1_BRK_UP_TRG_COM_IRQHandler,
	TIM1_CC_IRQHandler,
	TIM2_IRQHandler,
	TIM3_IRQHandler,
	TIM6_DAC_IRQHandler,
	TIM7_IRQHandler,
	TIM14_IRQHandler,
	TIM15_IRQHandler,
	TIM16_IRQHandler,
	TIM17_IRQHandler,
	I2C1_IRQHandler,
	I2C2_IRQHandler,
	SPI1_IRQHandler,
	SPI2_IRQHandler,
	USART1_IRQHandler,
	USART2_IRQHandler,
	USART3_4_IRQHandler,
	CEC_CAN_IRQHandler,
	USB_IRQHandler,
	BootRAM
};


