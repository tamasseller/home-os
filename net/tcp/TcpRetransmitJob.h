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
		// TODO
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

    	// TODO
    	uint16_t length = 123;

		return RetransmitJob::IpTxJob::startPreparation(launcher, item, socket->data.peerAddress, length, 0, 0);
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
