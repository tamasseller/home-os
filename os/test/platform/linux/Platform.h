/*
 * Platform.h
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "Profile.h"

typedef ProfileLinuxUm PlatformHardwareProfile;

static constexpr uintptr_t testStackSize = 1024*1024u;
static constexpr uintptr_t testStackCount = 10u;

#endif /* PLATFORM_H_ */
