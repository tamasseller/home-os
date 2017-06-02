/*
 * Platform.h
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "Profile.h"

typedef ProfileCortexM0 PlatformHardwareProfile;

static constexpr auto testStackSize = 128u;
static constexpr auto testStackCount = 8u;

#endif /* PLATFORM_H_ */
