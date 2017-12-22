/*
 * Interfaces.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef INTERFACES_H_
#define INTERFACES_H_

#include "Network.h"

#include "meta/ApplyToPack.h"

template<class S, class... Args>
class Network<S, Args...>::Interface: public Os::IoChannel, Pool::IoData, Os::IoJob {
    template<class> friend class NetworkTestAccessor;
	friend class Network<S, Args...>;

public:
	struct AddressResolver: Os::IoChannel {
		virtual const AddressEthernet& getAddress() = 0;
		virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) = 0;
		virtual void fillHeader(TxPacketBuilder&, const AddressEthernet& dst, uint16_t etherType) = 0;
	};

    inline Interface(uint16_t headerSize, AddressResolver* resolver): resolver(resolver), headerSize(headerSize) {}

	inline void ageContent() {}

	inline AddressResolver *getResolver() {
        return resolver;
    }

	inline typename Os::IoChannel *getSender() {
        return static_cast<typename Os::IoChannel*>(this);
    }

    inline uint16_t getHeaderSize() {
        return headerSize;
    }

    inline void requestRxBuffers(uint16_t n)
    {
        typename Pool::Allocator ret = state.pool.template allocateDirect<Pool::Quota::Rx>(n);

        if(!ret.hasMore()) {
            this->nBuffersRequested.increment(n);
            this->launch(&Interface::acquireBuffers);
        } else {
            this->takeRxBuffers(ret);
        }
	}

    virtual void takeRxBuffers(typename Pool::Allocator allocator) = 0;

    inline void ipPacketReceived(Packet packet, Interface* dev) {
    	struct ChecksumValidatorObserver: InetChecksumDigester {
    		size_t remainingHeaderLength;
    		size_t totalLength;
    		bool valid;

    		bool isDone() {
    			return !remainingHeaderLength;
    		}

    		void observeInternalBlock(char* start, char* end) {
    			if(!remainingHeaderLength)
    				return;

    			size_t length = end - start;

    			if(length < remainingHeaderLength) {
    				this->consume(start, length, remainingHeaderLength & 1);
    				remainingHeaderLength -= length;
    			}
    			else {
    				this->consume(start, remainingHeaderLength, remainingHeaderLength & 1);
    				remainingHeaderLength = 0;
    				valid = this->result() == 0;
    			}

    			totalLength += length;
    		}

    		void observeFirstBlock(char* start, char* end) {
    			remainingHeaderLength = (start[0] & 0xf) << 2;
        		totalLength = 0;
   				observeInternalBlock(start, end);
    		}
    	};

    	/*
    	 * Dispose of L2 header.
    	 */
    	packet.template dropInitialBytes<Pool::Quota::Rx>(this->getHeaderSize());

    	ObservedPacketStream<ChecksumValidatorObserver> reader;
    	reader.init(packet);

    	uint8_t versionAndHeaderLength;
    	if(!reader.read8(versionAndHeaderLength) || ((versionAndHeaderLength & 0xf0) != 0x40)) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	if(!reader.skipAhead(1)) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	uint16_t ipLength;
    	if(!reader.read16net(ipLength)) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	uint16_t id;
    	if(!reader.read16net(id)) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	uint16_t fragmentOffsetAndFlags;
    	if(!reader.read16net(fragmentOffsetAndFlags)) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	/*
    	 * Skip TTL.
    	 */
    	if(!reader.skipAhead(1)) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	uint8_t protocol;
    	if(!reader.read8(protocol)
    			|| (protocol != 1)) { // TODO add the other protocols.

    		// TODO send ICMP unreachable message.
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	if(!reader.skipAhead(6)) {
			packet.template dispose<Pool::Quota::Rx>();
			return;
		}

    	AddressIp4 dstIp;
    	if(!reader.read32net(dstIp.addr)) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	if(!reader.skipAhead(static_cast<uint16_t>(reader.remainingHeaderLength))) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	Os::assert(reader.isDone(), NetErrorStrings::unknown);
    	if(!reader.valid) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	/*
    	 * Check if the packet is for us.
    	 */
		if(Route* route = state.routingTable.findRouteWithSource(dev, dstIp)) {
			state.routingTable.releaseRoute(route);
		} else {
			/*
			 * If this host is not the final destination then, routing could be done here.
			 */
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
		}

    	if(protocol == 1) { // ICMP
	    	uint16_t typeCode;
	    	if(!reader.read16net(typeCode) ||
	    			(typeCode != 0x0800)) { // Echo request.
	    		// TODO add the other allowed ICMP message type+codes.
	        	packet.template dispose<Pool::Quota::Rx>();
	        	return;
	    	}
    	}

    	bool infinitePacket = reader.skipAhead(0xffff);
    	Os::assert(!infinitePacket, NetErrorStrings::unknown);

    	if(reader.totalLength != ipLength) {
        	packet.template dispose<Pool::Quota::Rx>();
        	return;
    	}

    	switch(protocol) {
    	case 1: // ICMP
    		state.icmpReplyJob.arrangeReply(packet);
    		break;
    	}
    }
private:
    using IoJob = typename Os::IoJob;
	using Result = typename IoJob::Result;
	using Launcher = typename IoJob::Launcher;
	using Callback = typename IoJob::Callback;

	AddressResolver* const resolver;
    typename Os::template Atomic<uint16_t> nBuffersRequested;
	uint16_t headerSize;

    static inline bool buffersAcquired(Launcher *launcher, IoJob* item, Result result)
    {
    	auto self = static_cast<Interface*>(item);
    	Os::assert(result == Result::Done, NetErrorStrings::unknown);

    	self->takeRxBuffers(static_cast<typename Pool::IoData*>(self)->allocator);

    	if(uint16_t n = self->nBuffersRequested.reset()) {
        	return state.pool.template allocateDirectOrDeferred<Pool::Quota::Rx>(
        			launcher,
        			&Interface::buffersAcquired,
        			static_cast<typename Pool::IoData*>(self),
        			n);
    	} else
    		return false;
    }

    static inline bool acquireBuffers(Launcher *launcher, IoJob* item)
    {
		auto self = static_cast<Interface*>(item);

		self->request.size = self->nBuffersRequested.reset();
        self->request.quota = Pool::Quota::Rx;

   		launcher->launch(&state.pool, &Interface::buffersAcquired, static_cast<typename Pool::IoData*>(self));
   		return true;
    }
};

template<class S, class... Args>
template<uint16_t blockMaxPayload, class... Input>
class Network<S, Args...>::Interfaces<blockMaxPayload, typename NetworkOptions::Set<Input...>, void>:
		public Input::template Wrapped<Network<S, Args...>, blockMaxPayload>...
{
    typedef void (*link)(Interfaces* const ifs);
    static void nop(Interfaces* const ifs) {}

    template<link rest, class C, class...>
	struct Initializer {
		static inline void init(Interfaces* const ifs) {
			static_cast<typename C::template Wrapped<Network<S, Args...>, blockMaxPayload>*>(ifs)->init();
			rest(ifs);
		}

		static constexpr link value = &init;
	};

    template<link rest, class C, class...>
	struct Ager {
		static inline void ageContent(Interfaces* const ifs) {
			static_cast<typename C::template Wrapped<Network<S, Args...>, blockMaxPayload>*>(ifs)->ageContent();
			rest(ifs);
		}

		static constexpr link value = &ageContent;
	};


public:
    inline void init() {
        (pet::ApplyToPack<link, Initializer, &Interfaces::nop, Input...>::value)(this);
	}

	inline void ageContent() {
        (pet::ApplyToPack<link, Ager, &Interfaces::nop, Input...>::value)(this);
	}
};

template<class S, class... Args>
template<template<class, uint16_t> class Driver>
constexpr inline auto* Network<S, Args...>::getEthernetInterface() {
	return static_cast<Ethernet<Driver<Network<S, Args...>, blockMaxPayload>>*>(&state.interfaces);
}

#endif /* INTERFACES_H_ */
