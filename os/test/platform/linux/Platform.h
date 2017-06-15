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

static constexpr auto testStackSize = 1024*1024u;
static constexpr auto testStackCount = 10u;

#endif /* PLATFORM_H_ */
