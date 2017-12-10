/*
 * SemaphoreProcess.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef SEMAPHOREPROCESS_H_
#define SEMAPHOREPROCESS_H_

template<class Os>
class SemaphoreProcess: public Os::template IoChannelBase<SemaphoreProcess<Os>>
{
	friend class SemaphoreProcess::IoChannelBase;

public:

	void init() {
		this->items.clear();
		count = 0;
		this->SemaphoreProcess::IoChannelBase::init();
	}

	struct Data: Os::IoJob::Data {
		Data *next, *prev;
		int param;
	};

	static SemaphoreProcess instance;

private:

	int count;
	pet::LinkedList<Data> items;

	bool hasJob() {
		return this->items.iterator().current() != nullptr;
	}

	bool addItem(typename Os::IoJob::Data* ioItem)
	{
		Data* item = static_cast<Data*>(ioItem);

		if(item->param < 0 && this->items.iterator().current())
			return this->items.addBack(item);

		while(item) {
			const int newVal = count + item->param;

			if(newVal < 0)
				return this->items.addBack(item);

			count = newVal;

			this->items.remove(item);
			this->jobDone(item);

			item = this->items.iterator().current();
		}

		return true;
	}

	bool removeCanceled(typename Os::IoJob::Data* item) {
		this->items.remove(static_cast<Data*>(item));
		return true;
	}

	void enableProcess() {}
	void disableProcess() {}
};

template<class Os>
SemaphoreProcess<Os> SemaphoreProcess<Os>::instance;




#endif /* SEMAPHOREPROCESS_H_ */
