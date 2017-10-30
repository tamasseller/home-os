/*
 * TestOsDefinitions.h
 *
 *      Author: tamas.seller
 */

#ifndef TESTOSDEFINITIONS_H_
#define TESTOSDEFINITIONS_H_

using OsRr = Scheduler<
        SchedulerOptions::HardwareProfile<PlatformHardwareProfile>,
        SchedulerOptions::SchedulingPolicy<RoundRobinPolicy>,
        SchedulerOptions::EnableAssert<true>
>;

using OsRrPrio = Scheduler<
        SchedulerOptions::HardwareProfile<PlatformHardwareProfile>,
        SchedulerOptions::SchedulingPolicy<RoundRobinPrioPolicy>,
        SchedulerOptions::EnableAssert<true>
>;

using OsRt4 = Scheduler<
        SchedulerOptions::HardwareProfile<PlatformHardwareProfile>,
        SchedulerOptions::SchedulingPolicy<RealtimePolicy<4>::Policy>,
        SchedulerOptions::EnableAssert<true>
>;

#endif /* TESTOSDEFINITIONS_H_ */
