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

#include "stm32f0xx.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_rcc.h"

#include "common/TestSuite.h"

#define LEDR_PORT GPIOC
#define LEDG_PORT GPIOC
#define LEDB_PORT GPIOC
#define LEDO_PORT GPIOC
#define LEDR_PIN GPIO_Pin_6
#define LEDG_PIN GPIO_Pin_9
#define LEDB_PIN GPIO_Pin_7
#define LEDO_PIN GPIO_Pin_8

void (*interruptRoutine)();

void TIM1_BRK_UP_TRG_COM_IRQHandler() {
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);

	void (*handler)() = interruptRoutine;

	if(handler)
		handler();
}

void initHw() {
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitTypeDef gpioInit;
	GPIO_StructInit(&gpioInit);
	gpioInit.GPIO_Pin  = LEDR_PIN | LEDG_PIN | LEDB_PIN | LEDO_PIN;
	gpioInit.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOC, &gpioInit);

	GPIO_WriteBit(LEDR_PORT, LEDR_PIN, Bit_SET);
	GPIO_WriteBit(LEDG_PORT, LEDG_PIN, Bit_SET);
	GPIO_WriteBit(LEDB_PORT, LEDB_PIN, Bit_SET);
	GPIO_WriteBit(LEDO_PORT, LEDO_PIN, Bit_SET);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	TIM_TimeBaseInitTypeDef timInit;
	TIM_TimeBaseStructInit(&timInit);
	TIM_TimeBaseInit(TIM1, &timInit);

	NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0);
	NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

	TIM_Cmd(TIM1, ENABLE);
}

void setFirstLed() {
	GPIO_WriteBit(LEDR_PORT, LEDR_PIN, Bit_SET);
	GPIO_WriteBit(LEDG_PORT, LEDG_PIN, Bit_RESET);
	GPIO_WriteBit(LEDB_PORT, LEDB_PIN, Bit_RESET);
	GPIO_WriteBit(LEDO_PORT, LEDO_PIN, Bit_RESET);
}

void blinkLeds(int n)
{
	const int pattern = (1 << (((n) >> 1) & 0x3)) | (1 << (((n + 1) >> 1) & 0x3));

	GPIO_WriteBit(LEDR_PORT, LEDR_PIN, (BitAction)(pattern & 0x1));
	GPIO_WriteBit(LEDB_PORT, LEDB_PIN, (BitAction)(pattern & 0x4));
	GPIO_WriteBit(LEDO_PORT, LEDO_PIN, (BitAction)(pattern & 0x2));
	GPIO_WriteBit(LEDG_PORT, LEDG_PIN, (BitAction)(pattern & 0x8));
}

void okBlink() {
	while(1) {
		GPIO_SetBits(GPIOC, LEDG_PIN);
		for(volatile int i = 0x7ffff; --i;);
		GPIO_ResetBits(GPIOC, LEDR_PIN | LEDG_PIN | LEDB_PIN | LEDO_PIN);
		for(volatile int i = 0x7ffff; --i;);
		GPIO_SetBits(GPIOC, LEDG_PIN);
		for(volatile int i = 0x7ffff; --i;);
		GPIO_ResetBits(GPIOC, LEDR_PIN | LEDG_PIN | LEDB_PIN | LEDO_PIN);
		for(volatile int i = 0x7ffff; --i;);
		GPIO_SetBits(GPIOC, LEDG_PIN);
		for(volatile int i = 0x7ffff; --i;);
		GPIO_ResetBits(GPIOC, LEDR_PIN | LEDG_PIN | LEDB_PIN | LEDO_PIN);
		for(volatile int i = 0x3fffff; --i;);

	}
}

void errorBlink() {
	while(1) {
		GPIO_SetBits(GPIOC, LEDR_PIN | LEDG_PIN | LEDB_PIN | LEDO_PIN);
		for(volatile int i = 0x1fffff; --i;);
		GPIO_ResetBits(GPIOC, LEDR_PIN | LEDG_PIN | LEDB_PIN | LEDO_PIN);
		for(volatile int i = 0x1fffff; --i;);
	}
}

void reqisterIrq(void (*newInterruptRoutine)()) {
	if(newInterruptRoutine) {
		interruptRoutine = newInterruptRoutine;
		TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	} else {
		TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);
		interruptRoutine = nullptr;
	}
}

int main()
{
	initHw();

	if(TestSuite<&blinkLeds, &setFirstLed, &reqisterIrq>::runTests(48000) == 0)
		okBlink();
	else
		errorBlink();
}
