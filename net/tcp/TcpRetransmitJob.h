/*
 * TcpRetransmitJob.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPRETRANSMITJOB_H_
#define TCPRETRANSMITJOB_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::TcpCore::RetransmitJob: public IpTxJob<RetransmitJob> {
	friend typename RetransmitJob::IpTxJob;

	pet::LinkedList<TcpSocket> sockets;

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload) {
		Fixup::tcpTxPostProcess(stream, checksum, payload);
	}

    inline bool restartIfNeeded(Launcher *launcher, IoJob* item)
    {
        if(!this->sockets.isEmpty()) {
            startReplySequence(launcher, item);
            return true;
        }

        return false;
    }

    inline bool onPrepFailed(Launcher *launcher, IoJob* item) {
    	// TODO
        return restartIfNeeded(launcher, item);
    }

    inline bool onSent(Launcher *launcher, IoJob* item, Result result) {
    	// TODO
    	return restartIfNeeded(launcher, item);
    }

	inline bool onPreparationDone(Launcher *launcher, IoJob* item)
	{
	    using namespace TcpPacket;

        auto socket = sockets.iterator().current();

        StructuredAccessor<SourcePort, DestinationPort, SequenceNumber, AcknowledgementNumber, Flags, WindowSize, Checksum, UrgentPointer> replyAccessor;
        replyAccessor.get<SourcePort>() =               socket->accessTag().getLocalPort();
        replyAccessor.get<DestinationPort>() =          socket->accessTag().getPeerPort();
        replyAccessor.get<SequenceNumber>() =           0; // TODO
        replyAccessor.get<AcknowledgementNumber>() =    0; // TODO
        replyAccessor.get<WindowSize>() =               0; // TODO
        replyAccessor.get<Checksum>() =                 0;
        replyAccessor.get<UrgentPointer>() =            0; // TODO
        replyAccessor.get<Flags>().clear();

        NET_ASSERT(replyAccessor.fill(this->accessPacket()));

	    switch(socket->state) {
            case TcpSocket::State::SynReceived:
                replyAccessor.get<Flags>().setDataOffset(20); // TODO mss option
                replyAccessor.get<Flags>().setSyn(true);
                replyAccessor.get<Flags>().setAck(true);
                break;
            default:
                break;
	    }

		return RetransmitJob::IpTxJob::template startTransmission<InetChecksumDigester>(
				launcher, item, IpProtocolNumbers::tcp, 0x40 /* TODO */);
	}

    inline bool onNoRoute(Launcher *launcher, IoJob* item) {
        return onPrepFailed(launcher, item);
    }

    inline bool onArpTimeout(Launcher *launcher, IoJob* item) {
        return onPrepFailed(launcher, item);
    }

    /*
     * These methods should never be called. (LCOV_EXCL_START)
     */

    inline bool onPreparationAborted(Launcher *launcher, IoJob* item, Result result) {
        NET_ASSERT(false);
        return false;
    }

    /*
     * These methods should never be called. (LCOV_EXCL_STOP)
     */

    static bool startReplySequence(Launcher *launcher, IoJob* item) {
    	auto self = static_cast<RetransmitJob*>(item);
    	auto socket = self->sockets.iterator().current();

        uint16_t length = 20;
    	switch(socket->state) {
    	    case TcpSocket::State::SynReceived:
    	        // TODO mss option
    	        break;
    	    default:
    	        break;
    	}

		return RetransmitJob::IpTxJob::startPreparation(launcher, item, socket->accessTag().getPeerAddress(), length, 0, 0);
    }

	/* TODO:
	 *		1. send an empty acknowledge packet with the current ack number
	 *		2. put packet into the acked list
	 */
public:
    void handle(TcpSocket &socket) {
    	this->sockets.addBack(&socket);
		this->launch(&RetransmitJob::startReplySequence);
    }
};

#endif /* TCPRETRANSMITJOB_H_ */
