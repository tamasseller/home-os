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
	PacketInputChannelBase<ConnectionTag>,
	Os::template SynchronousIoChannelBase<InputChannel>
{
	typedef typename InputChannel::PacketInputChannelBase RxChannel;
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
	inline RxChannel *getInputChannel() {
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
		using namespace TcpPacket;

		if(typename RxChannel::IoData* listener = this->findListener(tag)) {
			auto socket = static_cast<DataWaiter*>(listener)->getSocket();

			PacketStream reader(p);
	    	auto ipAccessor = Fixup::template extractAndSkip<PacketStream, IpPacket::Length>(reader);

	        StructuredAccessor<SequenceNumber, AcknowledgementNumber, Flags, WindowSize> tcpAccessor;

	        NET_ASSERT(tcpAccessor.extract(reader));

	        const auto rcvWnd = socket->lastAdvertisedReceiveWindow;
	        const auto rcvNxt = socket->expectedSequenceNumber;
	        const auto rcvEnd = rcvNxt + rcvWnd;

	        const auto segSeq = tcpAccessor.get<SequenceNumber>();
	        const auto segLen = ipAccessor.template get<IpPacket::Length>()
	                        		  - (ipAccessor.template get<IpPacket::Meta>().getHeaderLength()
	                        		  + tcpAccessor.get<Flags>().getDataOffset());
	        const auto segEnd = segSeq + segLen - 1;

	        const bool headOk = (rcvNxt <= segSeq) && (segSeq < rcvEnd);
	        const bool tailOk = (rcvNxt <= segEnd) && (segEnd < rcvEnd);

	        const bool acceptable = (segLen && !rcvWnd) ? false : (headOk || tailOk);

	        if(acceptable) {
	        	// TODO trim to window size (remove syn flag if trimmed off).
	        	if((rcvNxt == segSeq) && segLen) {
	        		if(!tcpAccessor.get<Flags>().hasRst()) {
	        			if(!tcpAccessor.get<Flags>().hasSyn()) {
	        				if(tcpAccessor.get<Flags>().hasAck()) {
	        					const auto sndNxt = socket->nextSequenceNumber;
	        					const auto sndUna = socket->lastReceivedAckNumber;

	        					const auto segAck = tcpAccessor.get<AcknowledgementNumber>();

	        					bool ok = true;

	        					switch(socket->state) {
									case TcpSocket::State::SynReceived:
										if(!((sndUna <= segAck) && (segAck <= sndNxt))) {
											// TODO "	If the segment acknowledgment is not acceptable,
											//			form a reset segment	"
											ok = false;
											break;
										}

										socket->state = TcpSocket::State::Established;
										/* no break to allow processing after state change */

									case TcpSocket::State::Established:
									case TcpSocket::State::CloseWait:
									case TcpSocket::State::FinWait1:
									case TcpSocket::State::FinWait2:
									case TcpSocket::State::Closing:
										if(sndNxt < segAck) {
											// TODO send ack
										} else if (segAck <= sndUna){
											// TODO handle dupack
										} else {
											socket->lastReceivedAckNumber = segAck;
											// TODO "	If SND.UNA < SEG.ACK =< SND.NXT, the send window
											//			should be updated.  If (SND.WL1 < SEG.SEQ or (SND.WL1
											//			= SEG.SEQ and SND.WL2 =< SEG.ACK)), set SND.WND <- SEG.WND,
											//			set SND.WL1 <- SEG.SEQ, and set SND.WL2 <- SEG.ACK.
											//			Note that SND.WND is an offset from SND.UNA, that SND.WL1
											//			records the sequence number of the last segment used to
											//			update SND.WND, and that SND.WL2 records the acknowledgment
											//			number of the last segment used to update SND.WND.	"
										}

										if(socket->state == TcpSocket::State::FinWait1) {
											// TODO "	if our FIN is now acknowledged then enter FIN-WAIT-2 and continue
											//			processing in that state.
										} else if(socket->state == TcpSocket::State::FinWait2) {
											// TODO "	if the retransmission queue is empty, the user's CLOSE can be
											//			acknowledged ("ok") but do not delete the TCB.
										} else if(socket->state == TcpSocket::State::Closing) {
											// TODO "	if the ACK acknowledges our FIN then enter the TIME-WAIT state,
											//			otherwise ignore the segment.
										} else if(socket->state == TcpSocket::State::LastAck) {
											// TODO "	The only thing that can arrive in this state is an
											//			acknowledgment of our FIN.  If our FIN is now acknowledged,
											//			delete the TCB, enter the CLOSED state, and return.
										}
										break;
									default:
										NET_ASSERT(false);
	        					}

								// TODO process data if in ESTABLISHED, FIN-WAIT-1 or FIN-WAIT-2, ignore otherwise.

								// TODO process FIN.

	        					if(ok) {
									// TODO put on ack queue.
									this->RxChannel::jobDone(listener);
									return true;
	        					}
	        				}
	        			} else {
	        				// TODO "	it is an error, send a reset, any outstanding RECEIVEs
	        				//			and SEND should receive "reset" responses, all segment
	        				//			queues should be flushed, the user should also receive
	        				//			an unsolicited general "connection reset" signal, enter
	        				//			the CLOSED state, delete the TCB, and return.
	        			}
	        		} else {
	        			switch(socket->state) {
	        			case TcpSocket::State::SynReceived:
							// TODO "	then return this connection to LISTEN state and return.
							// 			The user need not be informed.	"
	        				break;
	        			case TcpSocket::State::Established:
	        			case TcpSocket::State::FinWait1:
	        			case TcpSocket::State::FinWait2:
	        			case TcpSocket::State::CloseWait:
							// TODO "	All segment queues should be flushed.  Users should also
	        				//			receive an unsolicited general "connection reset" signal.
	        				//			Enter the CLOSED state, delete the TCB, and return.
	        				break;
	        			default:
	        				// TODO "	If the RST bit is set then, enter the CLOSED state, delete
	        				//			the TCB, and return.
	        				break;
	        			}
	        		}
	        	}
	        } else if(!tcpAccessor.get<Flags>().hasRst()) {
	        	// TODO put on ack queue.
	        }

	        state.increment(&DiagnosticCounters::Tcp::inputProcessed);
	        p.template dispose<Pool::Quota::Rx>();
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
