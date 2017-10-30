/*
 * SharedData.h
 *
 *      Author: tamas.seller
 */

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
