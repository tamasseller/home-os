/*
 * TcpReplyJobs.h
 *
 *  Created on: 2018.01.18.
 *      Author: tooma
 */

#ifndef TCPREPLYJOBS_H_
#define TCPREPLYJOBS_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::TcpRstJob: public IpReplyJob<TcpRstJob, InetChecksumDigester>
{
	typedef typename TcpRstJob::IpReplyJob Base;
	friend typename Base::IpTxJob;
	friend Base;

	inline void replySent() {
		state.increment(&DiagnosticCounters::Tcp::outputSent);
	}

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload) {
		Fixup::tcpTxPostProcess(stream, checksum, payload);
	}

	inline typename Base::InitialReplyInfo getReplyInfo(Packet& request)
	{
        AddressIp4 peerAddress;
        uint8_t ihl;

		PacketStream reader(request);

        NET_ASSERT(reader.read8(ihl));
        NET_ASSERT((ihl & 0xf0) == 0x40);

        NET_ASSERT(reader.skipAhead(11));
        NET_ASSERT(reader.read32net(peerAddress.addr));

		state.increment(&DiagnosticCounters::Tcp::rstPackets);

		return typename Base::InitialReplyInfo{peerAddress, static_cast<uint16_t>(20)};
	}

	inline typename Base::FinalReplyInfo generateReply(Packet& request, PacketBuilder& reply)
	{
		using namespace TcpConstants;
    	uint32_t seqNum, ackNum;
        uint16_t peerPort, localPort, payloadLength, flags;
        uint8_t ihl;

		PacketStream reader(request);

        NET_ASSERT(reader.read8(ihl));
        NET_ASSERT(reader.skipAhead(1));

        NET_ASSERT(reader.read16net(payloadLength));
        payloadLength = static_cast<uint16_t>(payloadLength - ((ihl & 0x0f) << 2));

        NET_ASSERT((ihl & 0xf0) == 0x40); // IPv4 only for now.
        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>((((ihl - 1) & 0x0f) << 2))));

        NET_ASSERT(reader.read16net(peerPort));
        NET_ASSERT(reader.read16net(localPort));
        NET_ASSERT(reader.read32net(seqNum));
        NET_ASSERT(reader.read32net(ackNum));
        NET_ASSERT(reader.read16net(flags));
        payloadLength = static_cast<uint16_t>(payloadLength - ((flags >> 10) & ~0x3));

        if(flags & synMask) payloadLength++;
        if(flags & finMask) payloadLength++;

		NET_ASSERT(reply.write16net(localPort));
		NET_ASSERT(reply.write16net(peerPort));

		NET_ASSERT(reply.write32net((flags & ackMask) ? ackNum : 0));		// seq number
		NET_ASSERT(reply.write32net(seqNum + payloadLength));				// ack number

		NET_ASSERT(reply.write16net((flags & ackMask) ? 0x5004: 0x5014));	// Flags
		NET_ASSERT(reply.write16net(0));		// Window size
		NET_ASSERT(reply.write32net(0));		// Checksum and urgent pointer.

		return typename Base::FinalReplyInfo{IpProtocolNumbers::tcp, 0xff};
	}
};

template<class S, class... Args>
class Network<S, Args...>::TcpRetransmitJob: public IpTxJob<TcpRetransmitJob> {
	friend typename TcpRetransmitJob::IpTxJob;

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
		return TcpRetransmitJob::IpTxJob::template startTransmission<InetChecksumDigester>(
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
    	auto self = static_cast<TcpRetransmitJob*>(item);
    	auto socket = self->sockets.iterator().current();

    	// TODO
    	uint16_t length = 123;

		return TcpRetransmitJob::IpTxJob::startPreparation(launcher, item, socket->data.peerAddress, length, 0, 0);
    }

	/* TODO:
	 *		1. send an empty acknowledge packet with the current ack number
	 *		2. put packet into the acked list
	 */
public:
    void handle(TcpSocket &socket) {
    	this->sockets.addBack(&socket);
		this->launch(&TcpRetransmitJob::startReplySequence);
    }
};

#endif /* TCPREPLYJOBS_H_ */
