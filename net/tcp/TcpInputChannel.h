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
		DataChain receivedData;
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

	inline bool takePacket(Packet p, ConnectionTag tag)
	{
		using namespace TcpPacket;

		if(typename RxChannel::IoData* listener = this->findListener(tag)) {
			auto receiver = static_cast<DataWaiter*>(listener);
			auto socket = receiver->getSocket();

	        /*
	         * RFC 793:
	         *
	         * 		"first check sequence number..."
	         */

			PacketStream reader(p);
	    	auto ipAccessor = Fixup::template extractAndSkip<PacketStream, IpPacket::Length>(reader);

	        StructuredAccessor<SequenceNumber, AcknowledgementNumber, Flags, WindowSize> tcpAccessor;

	        NET_ASSERT(tcpAccessor.extract(reader));

	        const auto headerLength = ipAccessor.template get<IpPacket::Meta>().getHeaderLength()
	        								+ tcpAccessor.get<Flags>().getDataOffset();

	        auto segSeq = tcpAccessor.get<SequenceNumber>();
	        auto segLen = ipAccessor.template get<IpPacket::Length>() - headerLength;
	        auto segEnd = segSeq + segLen - 1;

	        const auto rcvWnd = socket->lastAdvertisedReceiveWindow;
	        const auto rcvNxt = socket->expectedSequenceNumber;
	        const auto rcvEnd = rcvNxt + rcvWnd;

	        const bool headOk = (rcvNxt <= segSeq) && (segSeq < rcvEnd);
	        const bool tailOk = (rcvNxt <= segEnd) && (segEnd < rcvEnd);

	        const bool acceptable = (segLen && !rcvWnd) ? false : (headOk || tailOk);

	        enum class Action {Drop, Ack, Keep, Reset} action = Action::Drop;

	        if(acceptable) {
	        	auto headCutLength = headerLength;

	        	if(segSeq < rcvNxt) {
	        		// remove SYN flag as it is virtually trimmed off.
	        		tcpAccessor.get<Flags>().setSyn(false);

	        		const auto payloadCut = rcvNxt - segSeq;
	        		headCutLength += payloadCut;
	        		segLen -= payloadCut;
	        		segSeq = rcvNxt;
	        	}

				p.template dropInitialBytes<Pool::Quota::Rx>(headCutLength);

	        	if(rcvNxt < segEnd) {
	        		// TODO trim end if necessary
	        	}

	        	if(rcvNxt == segSeq) {
	    	        /*
	    	         * RFC 793:
	    	         *
	    	         * 		"second check the RST bit..."
	    	         */
	        		if(!tcpAccessor.get<Flags>().hasRst()) {
		    	        /*
		    	         * RFC 793:
		    	         *
		    	         *		"third check security and precedence" -> bullshit
		    	         *
		    	         * 		"fourth, check the SYN bit..."
		    	         */
	        			if(!tcpAccessor.get<Flags>().hasSyn()) {
	        				/*
	        				 * RFC 793:
	        				 *
	        				 *		"fifth check the ACK field..."
	        				 */
	        				if(tcpAccessor.get<Flags>().hasAck()) {
	        					const auto sndNxt = socket->nextSequenceNumber;
	        					const auto sndUna = socket->lastReceivedAckNumber;

	        					const auto segAck = tcpAccessor.get<AcknowledgementNumber>();

	        					switch(socket->state) {
									case TcpSocket::State::SynReceived:
										if(!((sndUna <= segAck) && (segAck <= sndNxt))) {
											action = Action::Reset;
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
											action = Action::Ack;
										} else if (segAck < sndUna){
											// TODO handle dupack
										} else {
											//
											// RFC 793:
											//
											//		"If SND.UNA < SEG.ACK =< SND.NXT, the send
											//		window should be updated."
											//
											if((sndUna < segAck) && (segAck <= sndNxt)) {
												//
												// RFC 793:
												//
												// 	"If (SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ
												//	 and SND.WL2 =< SEG.ACK)), set SND.WND <- SEG.WND,
												//   set SND.WL1 <- SEG.SEQ, and set SND.WL2 <- SEG.ACK."
												//
												const auto sndWl1 = socket->lastWindowUpdateSeqNum;
												const auto sndWl2 = socket->lastWindowUpdateAckNum;

												if(sndWl1 < segSeq || (sndWl1 == segSeq && sndWl2 <= segAck)) {
													socket->lastWindowUpdateSeqNum = segSeq;
													socket->lastWindowUpdateAckNum = segAck;
													socket->peerWindowSize = tcpAccessor.get<WindowSize>();
												}
											}

											socket->expectedSequenceNumber = segSeq + segLen;
											socket->lastReceivedAckNumber = segAck;
										}

										switch(socket->state) {
										case TcpSocket::State::FinWait1:
											// TODO "	if our FIN is now acknowledged then enter FIN-WAIT-2 and continue
											//			processing in that state.
											break;
										case TcpSocket::State::FinWait2:
											// TODO "	if the retransmission queue is empty, the user's CLOSE can be
											//			acknowledged ("ok") but do not delete the TCB.
											break;
										case TcpSocket::State::Closing:
											// TODO "	if the ACK acknowledges our FIN then enter the TIME-WAIT state,
											//			otherwise ignore the segment.											break;
										case TcpSocket::State::LastAck:
											// TODO "	The only thing that can arrive in this state is an
											//			acknowledgment of our FIN.  If our FIN is now acknowledged,
											//			delete the TCB, enter the CLOSED state, and return.											break;
										default:
											break;
										}

										break;
									default:
										NET_ASSERT(false);
	        					}

	        					/*
	        					 * RFC 793:
	        					 *
	        					 * 		"sixth, check the URG bit" -> nope.
	        					 *
	        					 * 		"seventh, process the segment text..."
	        					 */
	        					if(segLen) {
	        						switch(socket->state) {
									case TcpSocket::State::FinWait1:
									case TcpSocket::State::FinWait2:
									case TcpSocket::State::Established:
										action = Action::Keep;
										break;
									default:
										break;

	        						}

	        						/*
	        						 * RFC 793:
	        						 *
	        						 *		"eighth, check the FIN bit..."
	        						 */

	        						// TODO check FIN bit.
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
	        	} else {
	        		// TODO handle out of order segments.
	        	}
	        } else if(!tcpAccessor.get<Flags>().hasRst()) {
	        	action = Action::Ack;
	        }

	        switch(action) {
	        case Action::Ack:
	        	state.tcpCore.ackJob.handle(*socket);
		        /* no break here, to allow fall-through for cleanup */
	        case Action::Drop:
		        p.template dispose<Pool::Quota::Rx>();
	        	break;
	        case Action::Keep:
				receiver->receivedData.put(p);
				this->RxChannel::jobDone(listener);
				state.tcpCore.ackJob.handle(*socket);
	        	break;
	        case Action::Reset:
	        	state.tcpCore.rstJob.handlePacket(p);
	        	break;
	        }

	        state.increment(&DiagnosticCounters::Tcp::inputProcessed);
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
