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

    /**
     * The packet that is being read.
     *
     * The reference to the currently accessed packet needs to be copied,
     * in a synchronized context, to avoid concurrent access with the
     * handler of the reception of an incoming packet.
     */
    Packet packet;

    /**
     * Default (no-op) implemantation of packet preprocessing.
     *
     * Called after a new packet is fetched. Needs to be fast, as it is
     * invoked from the globally synchronized context.
     */
    inline void preprocess(Packet) {}

    /**
     * Default implementation of the reset method.
     *
     * The reset is called before a new packet is processed and after the last one.
     */
    inline void reset() {}

    /**
     * Default implementation of the packet disposal mechanism.
     *
     * Called after a packet is processed. Expected to take care of the further
     * life-cycle management of the packet.
     */
    inline void dispose(Packet packet) {
    	packet.template dispose<Pool::Quota::Rx>();
    }

    /**
     * Utility method to reset the internal state.
     */
    inline void invalidateAllStates() {
        static_cast<PacketStream*>(this)->invalidate();
        static_cast<Child*>(this)->reset();

		if(packet.isValid()) {
			static_cast<Child*>(this)->dispose(packet);
			packet.init(nullptr);
		}
    }

private:
     /**
     * Handler for completion of reception job.
     */
    static bool received(Launcher *launcher, IoJob* item, Result result)
    {
        auto self = static_cast<IpRxJob*>(item);

        if(result == Result::Canceled) {
        	self->invalidateAllStates();
            self->data.packets.template dropAll<Pool::Quota::Rx>();
            return false;
        }

        return true;
    }

public:
    inline IpRxJob() = default;
    inline IpRxJob(const Initializer&) { init(); }

    /**
     * Internal callback!
     *
     * It is called by IoRequest to decide whether it should block or not.
     */
    inline bool onBlocking()
    {
    	/*
    	 * Try to fetch a packet, this method is already contention protected context.
    	 */
		if(this->isInitialized()) {
			/*
			 * If we have a valid packet that is not consumed return true
			 * right away to indicate that there is data to be processed.
			 */
			if (!this->atEop())
				return false;

			invalidateAllStates();
		}

		/*
		 * If there is no packet (because either there was none or it have
		 * been dropped above) and a new one can be obtained then fetch it
		 * and return true to indicate that there is data to be processed.
		 */
		if(data.packets.take(packet)) { // RACE POINT: external synchronization required.
			static_cast<PacketStream*>(this)->init(packet);
			static_cast<Child*>(this)->preprocess(packet);
			return false;
		}

		/*
		 * If this point is reached then there is no data to be read and
		 * the originator of the request has to be blocked.
		 */
		return true;
    }

    /**
     * Initialize internal state to empty.
     */
    void init() {
    	packet.init(nullptr);
        invalidateAllStates();
    }

    /**
     * Launch point, for starting reception.
     */
    static bool startReception(Launcher *launcher, IoJob* item, Channel* channel)
    {
        auto self = static_cast<IpRxJob*>(item);

        NET_ASSERT(self->data.packets.isEmpty());

        launcher->launch(channel, &IpRxJob::received, &self->data);
        return true;
    }
};

#endif /* IPRXJOB_H_ */
