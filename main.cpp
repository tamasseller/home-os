/*
 * main.cpp
 *
 *  Created on: 2017.05.23.
 *      Author: tooma
 */

#include "stm32f0xx.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_rcc.h"

#include "Scheduler.h"
#include "RoundRobinPolicy.h"
#include "Profile.h"

typedef Scheduler<ProfileCortexM0, RoundRobinPolicy> Os;

#define LEDR_PORT PORTC
#define LEDG_PORT PORTC
#define LEDB_PORT PORTC
#define LEDO_PORT PORTC
#define LEDR_PIN GPIO_Pin_6
#define LEDG_PIN GPIO_Pin_9
#define LEDB_PIN GPIO_Pin_7
#define LEDO_PIN GPIO_Pin_8

template<class Child>
struct TaskHelper: public Os::Task {
	static uint32_t stack[128];
	TaskHelper(): Task(stack, sizeof(stack)) {}

	inline virtual ~TaskHelper() {}
};

template<class Child>
uint32_t TaskHelper<Child>::stack[128];


template<uint32_t period>
class Ticker {
	uint32_t next = 0;
public:
	inline bool check() {
		if((int)(next - Os::getTick()) < 0) {
			next += period;
			return true;
		}

		return false;
	}
};

struct T1: public TaskHelper<T1> {
	virtual void run() {
		Ticker<1000> t;
		for(;;) {
			if(t.check())
				GPIO_WriteBit(GPIOC, LEDB_PIN, (BitAction)!GPIO_ReadOutputDataBit(GPIOC, LEDB_PIN));
			else
				Os::yield();
		}
	}

	inline virtual ~T1() {}
} t1;

struct T2: public TaskHelper<T2> {
	virtual void run() {
		Ticker<800> t;
		for(;;) {
			if(t.check())
				GPIO_WriteBit(GPIOC, LEDO_PIN, (BitAction)!GPIO_ReadOutputDataBit(GPIOC, LEDO_PIN));
			else
				Os::yield();
		}
	}

	inline virtual ~T2() {}
} t2;

struct T3: public TaskHelper<T3> {
	virtual void run() {
		Ticker<1200> t;
		for(;;) {
			if(t.check())
				GPIO_WriteBit(GPIOC, LEDG_PIN, (BitAction)!GPIO_ReadOutputDataBit(GPIOC, LEDG_PIN));
			else
				Os::yield();
		}
	}

	inline virtual ~T3() {}
} t3;

struct T4: public TaskHelper<T4> {
	virtual void run() {
		Ticker<1500> t;
		for(;;) {
			if(t.check())
				GPIO_WriteBit(GPIOC, LEDR_PIN, (BitAction)!GPIO_ReadOutputDataBit(GPIOC, LEDR_PIN));
			else
				Os::yield();
		}
	}

	inline virtual ~T4() {}
} t4;

int main()
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitTypeDef gpioInit;
	GPIO_StructInit(&gpioInit);
	gpioInit.GPIO_Pin  = LEDR_PIN | LEDG_PIN | LEDB_PIN | LEDO_PIN;
	gpioInit.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOC, &gpioInit);

	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	SysTick_Config(48000);
	NVIC_SetPriority(SysTick_IRQn, 0);

	t1.start();
	t2.start();
	t3.start();
	t4.start();

	Os::start();

	for(;;) {}
}


