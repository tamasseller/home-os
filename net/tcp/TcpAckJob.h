/*
 * TcpAckJob.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPACKJOB_H_
#define TCPACKJOB_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::TcpCore::AckJob: public IpTxJob<AckJob> {
	friend typename AckJob::IpTxJob;

	pet::LinkedList<TcpSocket> sockets;

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload) {
		Fixup::tcpTxPostProcess(stream, checksum, payload);
	}

    inline bool restartIfNeeded(Launcher *launcher, IoJob* item)
    {
    	this->sockets.iterator().remove();

        if(!this->sockets.isEmpty()) {
            startReplySequence(launcher, item);
            return true;
        }

        return false;
    }

    inline bool onPrepFailed(Launcher *launcher, IoJob* item) {
    	this->AckJob::IpTxJob::disposePacket();
        return restartIfNeeded(launcher, item);
    }

    inline bool onSent(Launcher *launcher, IoJob* item, Result result) {
    	state.increment(&DiagnosticCounters::Tcp::outputSent);
    	this->AckJob::IpTxJob::disposePacket();
    	return restartIfNeeded(launcher, item);
    }

	inline bool onPreparationDone(Launcher *launcher, IoJob* item)
	{
	    using namespace TcpPacket;

        auto socket = sockets.iterator().current();
        auto accessor = socket->getTxHeader();

        accessor.template get<Flags>().clear();
        accessor.template get<Flags>().setDataOffset(20);
        accessor.template get<Flags>().setAck(true);

        NET_ASSERT(accessor.fill(this->accessPacket()));

		return AckJob::IpTxJob::template startTransmission<InetChecksumDigester>(launcher, item, IpProtocolNumbers::tcp, 0xff);
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
    	auto self = static_cast<AckJob*>(item);
    	auto socket = self->sockets.iterator().current();

        uint16_t length = 20;
    	switch(socket->state) {
    	    case TcpSocket::State::SynReceived:
    	        // TODO mss option
    	        break;
    	    default:
    	        break;
    	}

		return AckJob::IpTxJob::startPreparation(launcher, item, socket->accessTag().getPeerAddress(), length, 0, 0);
    }

public:
    void handle(TcpSocket &socket) {
    	this->sockets.addBack(&socket);
		this->launch(&AckJob::startReplySequence);
    }
};

#endif /* TCPACKJOB_H_ */
