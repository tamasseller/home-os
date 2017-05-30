/*
 * Os.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef OS_H_
#define OS_H_

#include "policy/RoundRobinPolicy.h"
#include "Scheduler.h"
#include "Profile.h"

typedef Scheduler<ProfileCortexM0, RoundRobinPolicy> Os;

static constexpr auto testStackSize = 128u;
static constexpr auto testStackCount = 8u;

#endif /* OS_H_ */
