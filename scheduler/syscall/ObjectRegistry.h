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

#ifndef OBJECTREGISTRY_H_
#define OBJECTREGISTRY_H_

#include "Scheduler.h"

#include "LimitedCTree.h"

namespace home {

template<class... Args>
template<class... Types>
struct Scheduler<Args...>::RegistryRootHub: Registry<Types>::Root... {
	template<class Type>
	typename Registry<Type>::Root* rootFor() {
		return static_cast<typename Registry<Type>::Root*>(this);
	}
};

/**
 * Per-type registry of arbitrary objects.
 *
 * Due to the characteristics of the LimitedCTree these methods
 * can be called both from system call handlers and application
 * interrupts enabling the system to detect any kind of invalid
 * pointer related errors on the system call interface.
 */
template<class... Args>
template<class Object>
class Scheduler<Args...>::ObjectRegistry<Object, true>
{
	public:
		/**
		 * Base class for objects that are stored in an _ObjectRegistry_.
		 */
		class ObjectBase: LimitedCTree::DualNode {
			friend class ObjectRegistry;
			friend LimitedCTree;
		};

	private:
		friend class Scheduler<Args...>;

		struct Root: LimitedCTree {};

		static inline int compare(const LimitedCTree::DualNode* x, const LimitedCTree::DualNode* y) {
			return (int)(x - y);
		}

		static bool add(Object* object) {
			LimitedCTree* tree = Scheduler<Args...>::state.template rootFor<Object>();
			return tree->add<&ObjectRegistry::compare>(object);
		}

		static bool remove(Object* object) {
			LimitedCTree* tree = Scheduler<Args...>::state.template rootFor<Object>();

			if(!tree->contains<&ObjectRegistry::compare>(object))
				return false;

			tree->remove(object);
			return true;
		}

	public:
		static inline void check(Object* object) {
			LimitedCTree* tree = Scheduler<Args...>::state.template rootFor<Object>();
			bool ok = tree->contains<&ObjectRegistry::compare>(object);
			Scheduler<Args...>::assert(ok, Scheduler<Args...>::ErrorStrings::invalidSyscallArgument);
		}

		static inline Object* lookup(uintptr_t objectPtr) {
			Object* ret = reinterpret_cast<Object*>(objectPtr);
			check(ret);
			return ret;
		}

		static inline uintptr_t getRegisteredId(Object* object) {
			return reinterpret_cast<uintptr_t>(object);
		}

		static void registerObject(Object* object) {
			Scheduler<Args...>::conditionalSyscall<SYSCALL(doRegisterObject<Object>)>(reinterpret_cast<uintptr_t>(object));
		}

		static void unregisterObject(Object* object) {
			Scheduler<Args...>::conditionalSyscall<SYSCALL(doUnregisterObject<Object>)>(reinterpret_cast<uintptr_t>(object));
		}
};

template<class... Args>
template<class Object>
class Scheduler<Args...>::ObjectRegistry<Object, false>
{
public:
	class ObjectBase {};
	struct Root {};

	static inline bool add(Object* object) {return false;}
	static inline bool remove(Object* object) {return false;}
	static inline void registerObject(Object* object) {}
	static inline void unregisterObject(Object* object) {}
	static inline void check(Object* object) {}

	static inline uintptr_t getRegisteredId(Object* object) {
		return reinterpret_cast<uintptr_t >(object);
	}

	static inline Object* lookup(uintptr_t objectPtr) {
		return reinterpret_cast<Object*>(objectPtr);
	}
};

template<class... Args>
template<class Object>
uintptr_t Scheduler<Args...>::doRegisterObject(uintptr_t objectPtr)
{
	bool ok = Registry<Object>::add(reinterpret_cast<Object*>(objectPtr));
	assert(ok, Scheduler<Args...>::ErrorStrings::objectAlreadyRegistered);
	return ok;
}

template<class... Args>
template<class Object>
uintptr_t Scheduler<Args...>::doUnregisterObject(uintptr_t objectPtr)
{
	return Registry<Object>::remove(reinterpret_cast<Object*>(objectPtr));
}

}

#endif /* OBJECTREGISTRY_H_ */
