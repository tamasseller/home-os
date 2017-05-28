/*
 * Internals.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef INTERNALS_H_
#define INTERNALS_H_

#include "Profile.h"

class ProfileCortexM0::Internals: ProfileCortexM0 {
	friend void SysTick_Handler();
	friend void PendSV_Handler();
	friend ProfileCortexM0;

	class Scb {
		friend void SysTick_Handler();
		friend ProfileCortexM0;
		class Icsr {
			static constexpr auto reg = ((volatile uint32_t *) 0xe000ed04);
			static constexpr uint32_t PendSvSet = 1u << 28;
		public:
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
			static constexpr auto shpr2 = ((volatile uint32_t *) 0xe000e01c);
			static constexpr auto shpr3 = ((volatile uint32_t *) 0xe000e010);

		public:
			static void init(uint8_t svcPrio, uint8_t sysTickPrio, uint8_t pendSvPrio) {
				*shpr2 = (*shpr2 & 0x00ffffff) | (svcPrio << 24);
				*shpr3 = (*shpr3 & 0x0000ffff) | (sysTickPrio << 24 | pendSvPrio << 16);
			}
		};
	};

	using ProfileCortexM0::Task;
	using ProfileCortexM0::Timer;
	using ProfileCortexM0::CallGate;
};

///////////////////////////////////////////////////////////////////////////////


#endif /* INTERNALS_H_ */
