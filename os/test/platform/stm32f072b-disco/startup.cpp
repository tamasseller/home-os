#include "stm32f0xx_rcc.h"
#include "stm32f0xx.h"
#include "stm32f0xx_misc.h"

#include <stdint.h>

extern "C" {
extern unsigned int sidata;
extern unsigned int sdata;
extern unsigned int edata;
extern unsigned int sbss;
extern unsigned int ebss;
extern unsigned int sstack;
extern unsigned int estack;

extern unsigned int preinit_array_start;
extern unsigned int preinit_array_end;
extern unsigned int init_array_start;
extern unsigned int init_array_end;
extern unsigned int fini_array_start;
extern unsigned int fini_array_end;
}

int main();
uint32_t SystemCoreClock = 48000000;

void Reset_Handler() {
	__IO uint32_t StartUpCounter = 0, HSEStatus = 0;

	RCC->CR |= (uint32_t) 0x00000001;

#if 1
	RCC->CFGR &= (uint32_t) 0xF8FFB80C;
#else
	RCC->CFGR &= (uint32_t)0x08FFB80C;
#endif

	RCC->CR &= (uint32_t) 0xFEF6FFFF;
	RCC->CR &= (uint32_t) 0xFFFBFFFF;
	RCC->CFGR &= (uint32_t) 0xFFC0FFFF;
	RCC->CFGR2 &= (uint32_t) 0xFFFFFFF0;
	RCC->CFGR3 &= (uint32_t) 0xFFFFFEAC;
	RCC->CR2 &= (uint32_t) 0xFFFFFFFE;
	RCC->CIR = 0x00000000;

	RCC->CR |= ((uint32_t)RCC_CR_HSEON);

	do {
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;
	} while ((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	if ((RCC->CR & RCC_CR_HSERDY) != RESET) {
		HSEStatus = (uint32_t) 0x01;
	} else {
		HSEStatus = (uint32_t) 0x00;
	}

	if (HSEStatus == (uint32_t) 0x01) {
		FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE_DIV1;
		RCC->CFGR &= (uint32_t)((uint32_t) ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_PREDIV1 | RCC_CFGR_PLLXTPRE_PREDIV1 | RCC_CFGR_PLLMULL6);
		RCC->CR |= RCC_CR_PLLON;

		while ((RCC->CR & RCC_CR_PLLRDY) == 0) { }

		RCC->CFGR &= (uint32_t)((uint32_t) ~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL) { }
	}else{
		for(;;){}
	}

	register unsigned int *in, *out;

	in = &sidata;
	out = &sdata;

	while(out < &edata)
		*out++ = *in++;

	out = &sstack;
	while(out < &estack)
		*out++ = 0x1baddeed;

	out = &sbss;
	while(out < &ebss)
		*out++ = 0;

	out = &preinit_array_start;
	while(out < &preinit_array_end){
		((void (*)(void))*out++)();
	}

	out = &init_array_start;
	while(out < &init_array_end){
		((void (*)(void))*out++)();
	}

	main();

	out = &fini_array_start;
	while(out < &fini_array_end){
		((void (*)(void))*out++)();
	}

	__asm volatile ("bkpt 0");
}
