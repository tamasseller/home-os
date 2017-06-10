/*
 * Scb.h
 *
 *  Created on: 2017.06.10.
 *      Author: tooma
 */

#ifndef SCB_H_
#define SCB_H_

namespace CortexCommon {

struct Scb {
	class Icsr {
		static constexpr auto reg = ((volatile uint32_t *) 0xe000ed04);
		static constexpr uint32_t PendSvSet = 1u << 28;
		static constexpr uint32_t VectactiveMask = 0x1f;
		static constexpr uint32_t VectactivePendSv = 14;

	public:
		static inline bool isInPendSV() {
			return (*reg & VectactiveMask) == VectactivePendSv;
		}

		static inline void triggerPendSV() {
			*reg = PendSvSet;
		}
	};

	class Syst {
		static constexpr auto ctrl = ((volatile uint32_t *) 0xe000e010);
		static constexpr auto load = ((volatile uint32_t *) 0xe000e014);
		static constexpr uint32_t SourceHclk  = 1u << 2;
		static constexpr uint32_t EnableIrq   = 1u << 1;
		static constexpr uint32_t EnableTimer = 1u << 0;

	public:

		static void init(uint32_t ticks) {
			*load = ticks-1;
			*ctrl |=  SourceHclk | EnableIrq | EnableTimer;
		}
	};

	class Shpr {
		static constexpr auto shpr2 = ((volatile uint32_t *) 0xe000ed1c);
		static constexpr auto shpr3 = ((volatile uint32_t *) 0xe000ed20);

	public:
		static void init(uint8_t svcPrio, uint8_t sysTickPrio, uint8_t pendSvPrio) {
			*shpr2 = (*shpr2 & 0x00ffffff) | (svcPrio << 24);
			*shpr3 = (*shpr3 & 0x0000ffff) | (sysTickPrio << 24 | pendSvPrio << 16);
		}
	};

	class Scr {
		static constexpr auto reg = ((volatile uint32_t *) 0xe000ed10);
		static constexpr uint32_t sevOnPend = 1u << 4;
		static constexpr uint32_t sleepDeep = 1u << 2;
		static constexpr uint32_t sleepOnExit = 1u << 1;

	public:
		static void configureSleepMode(bool doSevOnPend, bool doSleepDeep, bool doSleepOnExit) {
			*reg = (*reg & (~(sevOnPend | sleepDeep | sleepOnExit)))
					| ((doSevOnPend ? sevOnPend : 0)
					| (doSleepDeep ? sleepDeep : 0)
					| (doSleepOnExit ? sleepOnExit : 0));

		}
	};
};

}

#endif /* SCB_H_ */
