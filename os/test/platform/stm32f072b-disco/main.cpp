/*
 * main.cpp
 *
 *  Created on: 2017.05.23.
 *      Author: tooma
 */

#include "stm32f0xx.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_rcc.h"

#include "TestSuite.h"

#define LEDR_PORT GPIOC
#define LEDG_PORT GPIOC
#define LEDB_PORT GPIOC
#define LEDO_PORT GPIOC
#define LEDR_PIN GPIO_Pin_6
#define LEDG_PIN GPIO_Pin_9
#define LEDB_PIN GPIO_Pin_7
#define LEDO_PIN GPIO_Pin_8

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
}

void setFirstLed() {
	GPIO_WriteBit(LEDR_PORT, LEDR_PIN, Bit_SET);
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

int main()
{
	initHw();

	if(TestSuite<&blinkLeds, &setFirstLed>::runTests(48000))
		okBlink();
	else
		errorBlink();
}


