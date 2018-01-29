/*
 * TcpInputChannel.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPINPUTCHANNEL_H_
#define TCPINPUTCHANNEL_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::TcpCore::InputChannel:
	PacketInputChannel<ConnectionTag>,
	Os::template SynchronousIoChannelBase<InputChannel>
{
	typedef typename InputChannel::PacketInputChannel RxChannel;
	typedef typename Os::template SynchronousIoChannelBase<InputChannel> WindowChannel;
	friend WindowChannel;

	uint32_t tcpTime;

	struct TcpTimer {
		class TcpTimer* next;
		uint32_t deadline;
	};

	struct RetransmissionTimer: TcpTimer{};

public:
	struct WindowWaiter: public Os::IoJob::Data {
		uint16_t requestedWindowSize;
		inline TcpSocket* getSocket();
	};

	struct DataWaiter: public RxChannel::IoData {
		inline TcpSocket* getSocket();
	};

private:
	/**
	 * IoChannel implementation used for the WindowWaiter side.
	 */
	inline bool addItem(typename Os::IoJob::Data* data){
		// TODO check state
		return true;
	}

	/**
	 * IoChannel implementation used for the WindowWaiter side.
	 */
	inline bool removeCanceled(typename Os::IoJob::Data* data) {
		// TODO check state
		return true;
	}

public:
	inline PacketInputChannel<ConnectionTag> *getInputChannel() {
		return static_cast<RxChannel*>(this);
	}

	inline typename Os::template SynchronousIoChannelBase<InputChannel>* getWindowWaiterChannel() {
		return static_cast<WindowChannel*>(this);
	}

	void init() {
		getInputChannel()->init();
	}

	// If sequence number is correct:
	//   - update lastAcked, and window, drop from unacked as needed, reset retransmission timer.
	//   - if data length is not zero:
	//   	- check if data fits the receive window (if not drop packet silently),
	//   	- update seqnum and the window size with the length,
	//   	- queue the socket for ACK packet transmission,
	//   	- queue the data for reading
	//   	- notify the reader.
	//
	// If the sequence number is wrong:
	//   - increment fast retransmit dupack counter
	//   - queue the socket for ACK packet transmission (with the old sequence number),
	//   - drop packet.
	//
	inline bool takePacket(Packet p, ConnectionTag tag)
	{
		if(typename RxChannel::IoData* listener = this->findListener(tag)) {
			listener->packets.put(p);
			this->RxChannel::jobDone(listener);
			return true;
		}

		return false;
	}
};

template<class S, class... Args>
inline typename Network<S, Args...>::TcpSocket*
Network<S, Args...>::TcpCore::InputChannel::WindowWaiter::getSocket() {
	return static_cast<TcpSocket*>(this);
}

template<class S, class... Args>
inline typename Network<S, Args...>::TcpSocket*
Network<S, Args...>::TcpCore::InputChannel::DataWaiter::getSocket() {
	return static_cast<TcpSocket*>(this);
}

#endif /* TCPINPUTCHANNEL_H_ */
