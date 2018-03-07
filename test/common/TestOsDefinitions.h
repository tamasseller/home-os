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

#ifndef TESTOSDEFINITIONS_H_
#define TESTOSDEFINITIONS_H_

using OsRr = Scheduler<
        SchedulerOptions::HardwareProfile<PlatformHardwareProfile>,
        SchedulerOptions::PriorityLevels<4>,
        SchedulerOptions::EnableAssert<true>,
        SchedulerOptions::NumberOfSleepers<SchedulerOptions::ScalabilityHint::Many>
>;

using OsRrPrio = Scheduler<
        SchedulerOptions::HardwareProfile<PlatformHardwareProfile>,
        SchedulerOptions::PriorityLevels<4>,
        SchedulerOptions::EnableAssert<true>,
        SchedulerOptions::EnableRegistry<false>
>;

using OsRt4 = Scheduler<
        SchedulerOptions::HardwareProfile<PlatformHardwareProfile>,
        SchedulerOptions::PriorityLevels<4>,
        SchedulerOptions::EnableAssert<true>
>;

#endif /* TESTOSDEFINITIONS_H_ */
