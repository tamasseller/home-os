/*
 * IpRxJob.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef IPRXJOB_H_
#define IPRXJOB_H_

#include "Network.h"

template<class S, class... Args>
template<class Child, class Channel>
class Network<S, Args...>::IpRxJob: public Os::IoJob, public PacketStream {
protected:
    typename Channel::IoData data;

    inline void preprocess(Packet) {}

    inline void reset() {}

    inline void dispose(Packet packet) {
    	packet.template dispose<Pool::Quota::Rx>();
    }

    inline void invalidateAllStates() {
        static_cast<PacketStream*>(this)->invalidate();
        static_cast<Child*>(this)->reset();

        Packet packet;
		if(data.packets.take(packet))
			static_cast<Child*>(this)->dispose(packet);
    }

private:
    inline bool fetchPacket()
    {
        if(this->isInitialized()) {
        	/*
        	 * If we have a valid packet that is not consumed return true
        	 * right away to indicate that there is data to be processed.
        	 */
			if(!this->atEop())
				return true;

			invalidateAllStates();
        }

    	/*
    	 * If there is no packet (because either there was none or it have
    	 * been dropped above) and a new one can be obtained then fetch it
    	 * and return true to indicate that there is data to be processed.
    	 */
        Packet packet;
        if(data.packets.peek(packet)) {
			static_cast<PacketStream*>(this)->init(packet);
			static_cast<Child*>(this)->preprocess(packet);
			return true;
        }

        /*
         * If this point is reached then there is no data to be read.
         */
        return false;
    }

    static bool received(Launcher *launcher, IoJob* item, Result result)
    {
        auto self = static_cast<IpRxJob*>(item);

        if(result == Result::Done) {
            self->fetchPacket();
            return true;
        } else {
            Packet packet;

        	self->invalidateAllStates();

            self->data.packets.template dropAll<Pool::Quota::Rx>();
            return false;
        }
    }

public:
    // Internal!
    bool onBlocking()
    {
        return !fetchPacket();
    }

    void init() {
        invalidateAllStates();
    }

    static bool startReception(Launcher *launcher, IoJob* item, Channel* channel)
    {
        auto self = static_cast<IpRxJob*>(item);

        NET_ASSERT(self->data.packets.isEmpty());

        launcher->launch(channel, &IpRxJob::received, &self->data);
        return true;
    }
};

#endif /* IPRXJOB_H_ */
