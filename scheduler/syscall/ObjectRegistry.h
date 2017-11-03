/*
 * ObjectRegistry.h
 *
 *  Created on: 2017.10.28.
 *      Author: tooma
 */

#ifndef OBJECTREGISTRY_H_
#define OBJECTREGISTRY_H_

#include "Scheduler.h"

#include "LimitedCTree.h"

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
			Scheduler<Args...>::assert(ok, "Invalid object pointer as syscall argument");
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
	assert(ok, "Object registered multiple times");
	return ok;
}

template<class... Args>
template<class Object>
uintptr_t Scheduler<Args...>::doUnregisterObject(uintptr_t objectPtr)
{
	bool ok = Registry<Object>::remove(reinterpret_cast<Object*>(objectPtr));
	assert(ok || !state.isRunning, "Non-registered object unregistered");
	return ok;
}

#endif /* OBJECTREGISTRY_H_ */
