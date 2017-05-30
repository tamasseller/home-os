/*
 * cxx.cpp
 *
 *  Created on: 2017.05.04.
 *      Author: tooma
 */

extern "C" {
	int __aeabi_atexit(void (func)(void *), void* arg, void* d) {
		return 0;
	}

	void __cxa_pure_virtual() { while (1); }

	unsigned int __dso_handle;
}

void operator delete(void *)
{
}


