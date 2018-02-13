/*
 * PacketInputChannel.h
 *
 *  Created on: 2017.12.30.
 *      Author: tooma
 */

#ifndef PACKETINPUTCHANNEL_H_
#define PACKETINPUTCHANNEL_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>:: NullTag {
	bool operator ==(const NullTag& other) {
		return true;
	}
};

template<class S, class... Args>
class Network<S, Args...>:: DstPortTag {
	uint16_t n;
public:
	inline uint16_t getPortNumber() {
	    return n;
	}

	bool operator ==(const DstPortTag& other) {
		return n == other.n;
	}

	inline DstPortTag(uint16_t n): n(n) {}
	inline DstPortTag(): n(0) {}
};

template<class S, class... Args>
class Network<S, Args...>:: ConnectionTag {
	AddressIp4 peerAddress;
	uint16_t peerPort, localPort;
public:
	inline AddressIp4 getPeerAddress() {
	    return peerAddress;
	}

    inline uint16_t getPeerPort() {
        return peerPort;
    }

    inline uint16_t getLocalPort() {
        return localPort;
    }

	bool operator ==(const ConnectionTag& other) {
		return 	(peerAddress == other.peerAddress) &&
				(peerPort == other.peerPort) &&
				(localPort == other.localPort);
	}

	inline ConnectionTag(AddressIp4 peerAddress, uint16_t peerPort, uint16_t localPort):
			peerAddress(peerAddress), peerPort(peerPort), localPort(localPort) {}

	inline ConnectionTag(): peerAddress(AddressIp4::allZero), peerPort(0), localPort(0) {}
};

template<class S, class... Args>
template<class Tag>
class Network<S, Args...>::PacketInputChannelBase:
	public Os::template SynchronousIoChannelBase<PacketInputChannelBase<Tag>>
{
	friend class PacketInputChannelBase::SynchronousIoChannelBase;

public:
	class IoData: Tag, public Os::IoJob::Data {
		friend class PacketInputChannelBase;
		friend class pet::LinkedList<IoData>;
		IoData* next;

	public:
		Tag &accessTag() {
			return *this;
		}
	};

private:
	pet::LinkedList<IoData> items;

	inline bool addItem(typename Os::IoJob::Data* job)
	{
		return items.addBack(static_cast<IoData*>(job));
	}

	inline bool removeCanceled(typename Os::IoJob::Data* job)
	{
		return items.remove(static_cast<IoData*>(job));
		return true;
	}

protected:
	inline IoData* findListener(Tag t) {
		for(auto it = items.iterator(); it.current(); it.step())
			if(it.current()->accessTag() == t)
				return it.current();

		return nullptr;
	}

public:
	inline bool hasListener(Tag t = Tag())
	{
		return findListener(t) != nullptr;
	}
};


template<class S, class... Args>
template<class Tag>
class Network<S, Args...>::PacketInputChannel: public PacketInputChannelBase<Tag>
{
public:
	struct IoData: PacketInputChannel::PacketInputChannelBase::IoData {
		PacketChain packets;
	};

public:
	inline bool takePacket(Packet p, Tag t = Tag())
	{
		if(IoData* listener = static_cast<IoData*>(this->findListener(t))) {
			listener->packets.put(p);
			this->jobDone(listener);
			return true;
		}

		return false;
	}
};

#endif /* PACKETINPUTCHANNEL_H_ */
