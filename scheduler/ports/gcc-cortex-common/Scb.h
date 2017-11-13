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

#ifndef SCB_H_
#define SCB_H_

namespace home {

namespace CortexCommon {

struct Scb {
	class Icsr {
		static constexpr auto reg = 0xe000ed04;
		static constexpr uint32_t PendSvSet = 1u << 28;
		static constexpr uint32_t PendSvClr = 1u << 27;
		static constexpr uint32_t VectactiveMask = 0x1f;
		static constexpr uint32_t VectactivePendSv = 14;

	public:
		static inline bool isInPendSV() {
			return (*((uintptr_t*)reg) & VectactiveMask) == VectactivePendSv;
		}

		static inline void triggerPendSV() {
			*((uintptr_t*)reg) = PendSvSet;
		}

		static inline void clearPendSV() {
			*((uintptr_t*)reg) = PendSvClr;
		}
	};

	class Syst {
		static constexpr auto ctrl = 0xe000e010;
		static constexpr auto load = 0xe000e014;
		static constexpr uint32_t SourceHclk  = 1u << 2;
		static constexpr uint32_t EnableIrq   = 1u << 1;
		static constexpr uint32_t EnableTimer = 1u << 0;

	public:

		static void init(uint32_t ticks) {
			*((uintptr_t*)load) = ticks-1;
			*((uintptr_t*)ctrl) |= SourceHclk | EnableIrq | EnableTimer;
		}

		static void disable() {
			*((uintptr_t*)ctrl) &= ~(EnableIrq | EnableTimer);
		}
	};

	class Shpr {
		static constexpr auto shpr2 = 0xe000ed1c;
		static constexpr auto shpr3 = 0xe000ed20;

	public:
		static void init(uint8_t svcPrio, uint8_t sysTickPrio, uint8_t pendSvPrio) {
			*((uintptr_t*)shpr2) = (*((uintptr_t*)shpr2) & 0x00ffffff) | (svcPrio << 24);
			*((uintptr_t*)shpr3) = (*((uintptr_t*)shpr3) & 0x0000ffff) | (sysTickPrio << 24 | pendSvPrio << 16);
		}
	};

	class Scr {
		static constexpr auto reg = 0xe000ed10;
		static constexpr uint32_t sevOnPend = 1u << 4;
		static constexpr uint32_t sleepDeep = 1u << 2;
		static constexpr uint32_t sleepOnExit = 1u << 1;

	public:
		static void configureSleepMode(bool doSevOnPend, bool doSleepDeep, bool doSleepOnExit) {
			*((uintptr_t*)reg) = (*((uintptr_t*)reg) & (~(sevOnPend | sleepDeep | sleepOnExit)))
					| ((doSevOnPend ? sevOnPend : 0)
					| (doSleepDeep ? sleepDeep : 0)
					| (doSleepOnExit ? sleepOnExit : 0));

		}
	};
};

}

}

#endif /* SCB_H_ */
