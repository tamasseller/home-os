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
template<class Tag>
class Network<S, Args...>::PacketInputChannel: public Os::template SynchronousIoChannelBase<PacketInputChannel<Tag>>
{
	friend class PacketInputChannel::SynchronousIoChannelBase;

public:
	class IoData: Tag, public Os::IoJob::Data {
		friend class PacketInputChannel;
		friend class pet::LinkedList<IoData>;
		IoData* next;

		Tag &accessTag() {
			return *this;
		}

	public:
		PacketQueue packets;
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

public:
	inline bool takePacket(Packet p, Tag t = Tag())
	{
		for(auto it = items.iterator(); it.current(); it.step()) {
			if(it.current()->accessTag() == t) {
				it.current()->packets.putPacketChain(p);
				this->jobDone(it.current());
				return true;
			}
		}

		return false;
	}
};

#endif /* PACKETINPUTCHANNEL_H_ */
