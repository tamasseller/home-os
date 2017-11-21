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

#ifndef DUMMYPROCESS_H_
#define DUMMYPROCESS_H_

#include "CommonTestUtils.h"

template<class Os>
class DummyProcess: public Os::template IoChannelBase<DummyProcess<Os>> {
public:
	volatile unsigned int counter;

	static DummyProcess instance;

	void init() {
		this->jobs.clear();
		counter = 0;
		this->DummyProcess::IoChannelBase::init();
	}

private:
	friend class DummyProcess::IoChannelBase;

	static inline void processWorkerIsr() {
		if(instance.counter) {
			typename Os::IoJob* job = instance.jobs.front();
			int* param = reinterpret_cast<int*>(job->param);

			auto count = instance.counter--;

			if(param)
				*param = count;

			instance.jobDone(job);
		}
	}

	bool addJob(typename Os::IoJob* job) {
		return this->jobs.addBack(job);
	}

	bool removeJob(typename Os::IoJob* job) {
		return this->jobs.remove(job);
	}

	void enableProcess() {
		CommonTestUtils::registerIrq(&DummyProcess::processWorkerIsr);
	}

	void disableProcess() {
		CommonTestUtils::registerIrq(nullptr);
	}
};

template<class Os>
DummyProcess<Os> DummyProcess<Os>::instance;

#endif /* DUMMYPROCESS_H_ */
