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

#ifndef SHAREDTABLE_H_
#define SHAREDTABLE_H_

#include "DataContainer.h"

template<class Os, class Data, size_t nItems>
struct SharedTable
{
        class Entry: Os::template Atomic<uintptr_t>, DataContainer<Data> {
            friend class SharedTable;

            static constexpr uintptr_t validFlag = (uintptr_t)1 << (sizeof(uintptr_t) * 8 - 1);

            typename Os::template Atomic<uintptr_t> &getFlags() {
                return *this;
            }

            bool takeIfValidOrEmpty() {
                bool ok;

                getFlags()([&ok](uintptr_t old, uintptr_t &result){
                    result = old + 1;
                    return ok = !old || (old & validFlag);
                });

                return ok;
            }

            bool takeIfValid() {
                bool ok;

                getFlags()([&ok](uintptr_t old, uintptr_t &result){
                    result = old + 1;
                    return ok = old & validFlag;
                });

                return ok;
            }

            Entry(const Entry&) = delete;
            Entry(Entry&&) = delete;
            void operator =(const Entry&) = delete;
            void operator =(Entry&&) = delete;
            Entry() = default;

        public:
            inline Data& access() {
                return *(static_cast<DataContainer<Data>*>(this)->getData());
            }

            inline void erase() {
                auto ret = getFlags()([](uintptr_t old, uintptr_t &result){
                    result = (old - 1) & ~validFlag;
                    return true;
                });

                if(ret == (validFlag | 1)) {
                    this->destroy();
                    getFlags() = 0;
                }
            }

            inline void release() {
                if(getFlags() == 1) {
                    this->destroy();
                    getFlags() = 0;
                } else {
                    getFlags()([](uintptr_t old, uintptr_t &result){
                        result = old - 1;
                        return true;
                    });
                }
            }

            inline void finalize() {
                // assert(getFlags() == 1)
                getFlags() = validFlag;
            }

            inline bool isValid() {
                return ((uintptr_t)getFlags()) & validFlag;
            }

            using DataContainer<Data>::init;
        };

    private:

        Entry entries[nItems];

    public:
        static inline Entry* entryFromData(Data* data) {
        	return static_cast<Entry*>(reinterpret_cast<DataContainer<Data>*>(data));
        }

        template<class C>
        inline Entry* findOrAllocate(C&& c)
        {
            Entry* empty = nullptr;

            for(Entry* entry = entries; entry < entries + nItems; entry++) {
                if(!empty){
                    if(entry->takeIfValidOrEmpty()) {
                        if(entry->isValid()) {
                            if(c(&entry->access()))
                                return entry;
                            else
                                entry->release();
                        } else {
                            empty = entry;
                        }
                    }
                } else {
                    if(entry->takeIfValid()) {
                        if(c(&entry->access())) {
                            empty->getFlags() = 0;
                            return entry;
                        }

                        entry->release();
                    }
                }
            }

            return empty;
        }

        template<class C>
        inline Entry* findBest(C&& c)
        {
            Entry* best = nullptr;
            uintptr_t metric = 0;

            for(Entry* entry = entries; entry < entries + nItems; entry++) {
                if(entry->takeIfValid()) {
                    if(uintptr_t newMetric = c(&entry->access())) {
                        if(newMetric > metric) {
                            if(best)
                                best->release();

                            best = entry;
                            metric = newMetric;
                        } else
                            entry->release();
                    } else
                        entry->release();
                }
            }

            return best;
        }

        template<class C>
        inline Entry* find(C&& c)
        {
            for(Entry* entry = entries; entry < entries + nItems; entry++) {
                if(entry->takeIfValid()) {
                    if(c(&entry->access()))
                        return entry;

                    entry->release();
                }
            }

            return nullptr;
        }
};

#endif /* SHAREDTABLE_H_ */
