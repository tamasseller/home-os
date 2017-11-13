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

#ifndef SHAREDDATA_H_
#define SHAREDDATA_H_

template<uintptr_t size>
class SharedData {
    uint16_t data[size];
    bool error = false;
public:
    inline void update() {
        for(int i = sizeof(data)/sizeof(data[0]) - 1; i >= 0; i--) {
            if(data[i] != data[0])
                error = true;

            data[i]++;
        }
    }

    inline bool check(uint16_t value) {
        for(int i = sizeof(data)/sizeof(data[0]) - 1; i >= 0; i--)
            if(data[i] != value)
                return false;

        return !error;
    }

    inline void reset() {
        for(int i = sizeof(data)/sizeof(data[0]) - 1; i >= 0; i--)
            data[i] = 0;

        error = false;
    }
};

#endif /* SHAREDDATA_H_ */
