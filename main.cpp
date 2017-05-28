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
struct TaskHelper: public Os::Task<Child> {
	static uint32_t stack[128];
	void start() {
		Os::Task<Child>::start(stack, sizeof(stack));
	}
};

template<class Child>
uint32_t TaskHelper<Child>::stack[128];


template<uint32_t period>
class Ticker {
	uint32_t next = 0;
public:
	Ticker(): next(Os::getTick()) {}
	inline bool check() {
		if((int)(next - Os::getTick()) < 0) {
			GPIO_SetBits(GPIOC, LEDB_PIN);
			GPIO_ResetBits(GPIOC, LEDB_PIN);
		}

		return false;
	}
};

Os::Mutex mutex;

struct T1: public TaskHelper<T1> {
	void run() {
		while(1) {
			mutex.lock();
			GPIO_SetBits(GPIOC, LEDR_PIN);
			for(volatile int i = 0xfffff; --i;);
			GPIO_ResetBits(GPIOC, LEDR_PIN);
			mutex.unlock();
			for(volatile int i = 0xfffff; --i;);
		}

	}
} t1;

struct T2: public TaskHelper<T2> {
	void run() {
		while(1) {
			mutex.lock();
			GPIO_SetBits(GPIOC, LEDB_PIN);
			Os::sleep(1000);
			GPIO_ResetBits(GPIOC, LEDB_PIN);
			mutex.unlock();
			Os::sleep(1000);
		}
	}
} t2;


struct T3: public TaskHelper<T3> {
	void run() {
		while(1) {
			Os::sleep(200);
			GPIO_SetBits(GPIOC, LEDG_PIN);
			Os::sleep(200);
			GPIO_ResetBits(GPIOC, LEDG_PIN);
		}
	}
} t3;

struct Idle: public TaskHelper<Idle> {
	void run() {
		for(;;);
	}
} idle;

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

	mutex.init();
	idle.start();
	t1.start();
	t2.start();
	t3.start();
	Os::start(48000);

	GPIO_ResetBits(GPIOC, LEDR_PIN | LEDG_PIN | LEDB_PIN | LEDO_PIN);

	for(;;) {}
}


