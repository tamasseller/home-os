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

/**
 * Per-type registry of arbitrary objects.
 *
 * Due to the characteristics of the LimitedCTree these methods
 * can be called both from system call handlers and application
 * interrupts enabling the system to detect any kind of invalid
 * pointer related errors on the system call interface.
 */
template<class... Options>
template<class Object>
class Scheduler<Options...>::ObjectRegistry<Object, true>
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
		template<class> friend uintptr_t Scheduler<Options...>::doRegisterObject(uintptr_t objectPtr);
		template<class> friend uintptr_t Scheduler<Options...>::doUnregisterObject(uintptr_t objectPtr);

		static LimitedCTree tree;

		static inline int compare(const LimitedCTree::DualNode* x, const LimitedCTree::DualNode* y) {
			return (int)(x - y);
		}

		static bool add(Object* object) {
			return tree.add<&ObjectRegistry::compare>(object);
		}

		static bool remove(Object* object) {
			if(!tree.contains<&ObjectRegistry::compare>(object))
				return false;

			tree.remove(object);
			return true;
		}

	public:
		static inline void check(Object* object) {
			bool ok = tree.contains<&ObjectRegistry::compare>(object);
			Scheduler<Options...>::assert(ok, "Invalid object pointer as syscall argument");
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
			Scheduler<Options...>::conditionalSyscall(&Scheduler<Options...>::doRegisterObject<Object>, reinterpret_cast<uintptr_t>(object));
		}

		static void unregisterObject(Object* object) {
			Scheduler<Options...>::conditionalSyscall(&Scheduler<Options...>::doUnregisterObject<Object>, reinterpret_cast<uintptr_t>(object));
		}
};

template<class... Options>
template<class Object>
class Scheduler<Options...>::ObjectRegistry<Object, false>
{
public:
	class ObjectBase {};

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


template<class... Options>
template<class T>
LimitedCTree Scheduler<Options...>::ObjectRegistry<T, true>::tree;

template<class... Options>
template<class Object>
uintptr_t Scheduler<Options...>::doRegisterObject(uintptr_t objectPtr)
{
	bool ok = Registry<Object>::add(reinterpret_cast<Object*>(objectPtr));
	assert(ok, "Object registered multiple times");
	return ok;
}

template<class... Options>
template<class Object>
uintptr_t Scheduler<Options...>::doUnregisterObject(uintptr_t objectPtr)
{
	bool ok = Registry<Object>::remove(reinterpret_cast<Object*>(objectPtr));
	assert(ok, "Non-registered object unregistered");
	return ok;
}

#endif /* OBJECTREGISTRY_H_ */
