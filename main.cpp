/*
 * main.cpp
 *
 *  Created on: 2017.05.23.
 *      Author: tooma
 */

#include "stm32f0xx.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_rcc.h"

#define LEDR_PORT PORTC
#define LEDG_PORT PORTC
#define LEDB_PORT PORTC
#define LEDO_PORT PORTC
#define LEDR_PIN GPIO_Pin_6
#define LEDG_PIN GPIO_Pin_9
#define LEDB_PIN GPIO_Pin_7
#define LEDO_PIN GPIO_Pin_8

struct Task
{
	static Task* current;
	static void* select(void* sp) {
		current->sp = sp;
		current = current->next;
		return current->sp;
	}

	__attribute__((naked))
	static void entry(Task* self) {
		self->run();
	}

	static void exit() {
	}


	void yield() {
		*((volatile uint32_t *)0xe000ed04) = 0x10000000;
	}

	void* sp;
	Task* next;

	virtual void run() = 0;

	inline Task(void* task, uint32_t length) {
		uint32_t* data = (uint32_t*)task + (length / 4 - 1);

		*data-- = 0x01000000;				// xpsr
		*data-- = (uint32_t)&Task::entry;	// pc
		*data-- = (uint32_t)&Task::exit;	// lr
		*data-- = 0xbaadf00d;				// r12
		*data-- = 0xbaadf00d;				// r3
		*data-- = 0xbaadf00d;				// r2
		*data-- = 0xbaadf00d;				// r1
		*data-- = (uint32_t)this;			// r0
		*data-- = 0xbaadf00d;				// r11
		*data-- = 0xbaadf00d;				// r10
		*data-- = 0xbaadf00d;				// r9
		*data-- = 0xbaadf00d;				// r8
		sp = data+1;
		*data-- = 0xbaadf00d;				// r7
		*data-- = 0xbaadf00d;				// r6
		*data-- = 0xbaadf00d;				// r5
		*data-- = 0xbaadf00d;				// r4
	}

	void start() {
		current = this;
		asm volatile (
			"msr psp, %0		\n"
			"movs r0, #2		\n"
			"msr CONTROL, r0	\n"
			"pop {r0-r5}		\n"
			"mov lr, r5			\n"
			"cpsie i			\n"
			"pop {pc}			\n"
				: /* No outputs */
				: "r" ((uint32_t*)sp + 4)
				: /* No clobbers */
		);
	}

	inline virtual ~Task() {}
};

Task* Task::current;


__attribute__((naked))
void PendSV_Handler() {
	asm volatile (
		"mrs r0, psp		\n"
		"sub r0, r0, #32	\n"
		"stmia r0!, {r4-r7}	\n" /* Store remaining low regs */
		"mov r1, r0			\n"	/* Save pointer between high and low regs */
		"mov r4, r8			\n"
		"mov r5, r9			\n"
		"mov r6, r10		\n"
		"mov r7, r11		\n"
		"stmia r1!, {r4-r7}	\n" /* Store remaining high regs */

		"mov r4, lr			\n"	/* Store return address in r5 */
		"bl %c0				\n"	/* Call task selector */
		"mov lr, r4			\n"	/* Restore return address in lr */

		"ldmia r0!, {r4-r7}	\n" /* Load high regs*/
		"mov r8, r4			\n"
		"mov r9, r5			\n"
		"mov r10, r6		\n"
		"mov r11, r7		\n"

		"msr psp, r0		\n" /* Save original top of stack */
		"sub r0, r0, #32	\n" /* Go back down for the low regs */
		"ldmia r0!, {r4-r7}	\n" /* Load low regs*/
		"bx lr				\n" /* Return */
			: /* No outputs */
			: "X" (Task::select)
			: /* No clobbers */
	);
}


template<class Child>
struct TaskHelper: public Task {
	static uint32_t stack[128];
	TaskHelper(): Task(stack, sizeof(stack)) {}

	inline virtual ~TaskHelper() {}
};

template<class Child>
uint32_t TaskHelper<Child>::stack[128];

volatile uint32_t tick = 0;
void SysTick_Handler()
{
	tick++;
}

template<uint32_t period>
class Ticker {
	uint32_t next = 0;
public:
	inline bool check() {
		if((int)(next - tick) < 0) {
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
				yield();
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
				yield();
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
				yield();
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
				yield();
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

	t1.next = &t4;
	t2.next = &t1;
	t3.next = &t2;
	t4.next = &t3;

	t1.start();

	for(;;) {}
}


