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

#ifndef EVENT_H_
#define EVENT_H_

namespace home {

template<class... Args>
class Scheduler<Args...>::Event: SharedAtomicList::Element
{
	friend EventList;

public:
	typedef void (*Callback)(Event*, uintptr_t);

private:
	const Callback callback;

protected:
	inline Event(void (* callback)(Event*, uintptr_t)): callback(callback) {}

public:
	inline Callback getCallback() const {
		return callback;
	}
};

}

#endif /* EVENT_H_ */
