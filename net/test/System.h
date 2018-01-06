/*
 * System.h
 *
 *  Created on: 2018.01.04.
 *      Author: tooma
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "Scheduler.h"
#include "Network.h"

#include "ports/gcc-linux-um/Profile.h"

#include "LinuxTapDevice.h"

using Os = home::Scheduler<
		home::SchedulerOptions::HardwareProfile<home::ProfileLinuxUm>,
		home::SchedulerOptions::SchedulingPolicy<home::RoundRobinPolicy>,
		home::SchedulerOptions::EnableAssert<true>,
		home::SchedulerOptions::NumberOfSleepers<home::SchedulerOptions::ScalabilityHint::Many>
>;

using Net = Network<
		Os,
		NetworkOptions::BufferSize<64>,
		NetworkOptions::BufferCount<256>,
		NetworkOptions::TicksPerSecond<1000>,
		NetworkOptions::Interfaces<
		    NetworkOptions::Set<
		        NetworkOptions::EthernetInterface<LinuxTapDevice>
		    >
		>
>;



#endif /* SYSTEM_H_ */
