/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#include <stdint.h>

extern "C" void DefaultHandler() {
    for(;;){}
}

extern "C" void DefaultHardFaultHandler(){
    for(;;){}
}

extern "C" void estack();
void Reset_Handler() __attribute__((weak, alias("DefaultHandler")));
void NMI_Handler() __attribute__((weak, alias("DefaultHandler")));
void HardFault_Handler() __attribute__((weak, alias("DefaultHardFaultHandler")));
void SVC_Handler() __attribute__((weak, alias("DefaultHandler")));
void PendSV_Handler() __attribute__((weak, alias("DefaultHandler")));
void SysTick_Handler() __attribute__((weak, alias("DefaultHandler")));

void WWDG_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void PVD_VDDIO2_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void RTC_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void FLASH_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void RCC_CRS_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void EXTI0_1_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void EXTI2_3_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void EXTI4_15_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TSC_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void DMA1_Channel1_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void DMA1_Channel2_3_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void DMA1_Channel4_5_6_7_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void ADC1_COMP_IRQHandler () __attribute__((weak, alias("DefaultHandler")));
void TIM1_BRK_UP_TRG_COM_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TIM1_CC_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TIM2_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TIM3_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TIM6_DAC_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TIM7_IRQHandler    () __attribute__((weak, alias("DefaultHandler")));
void TIM14_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TIM15_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TIM16_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void TIM17_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void I2C1_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void I2C2_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void SPI1_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void SPI2_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void USART1_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void USART2_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void USART3_4_IRQHandler () __attribute__((weak, alias("DefaultHandler")));
void CEC_CAN_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void USB_IRQHandler() __attribute__((weak, alias("DefaultHandler")));
void BootRAM    () __attribute__((weak, alias("DefaultHandler")));

typedef void (*funcp)();

__attribute__((section(".isr_vector")))
funcp vector_table[] = {
	(funcp)estack, /* &_estack*/
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


