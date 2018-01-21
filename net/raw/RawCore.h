/*
 * RawCore.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef RAWCORE_H_
#define RAWCORE_H_

template<class S, class... Args>
struct Network<S, Args...>::RawCore: Network<S, Args...>::RxPacketHandler {
	class EchoReplyJob;
	typedef PacketInputChannel<NullTag> InputChannel;
	InputChannel inputChannel;

	void init() {
		inputChannel.init();
	}

	virtual void handlePacket(Packet packet)
	{
		if(!inputChannel.takePacket(packet))
			packet.template dispose<Pool::Quota::Rx>();
	}
};

#endif /* RAWCORE_H_ */
