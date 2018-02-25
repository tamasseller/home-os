/*
 * TcpTx.h
 *
 *  Created on: 2018.01.29.
 *      Author: tooma
 */

#ifndef TCPTX_H_
#define TCPTX_H_

#include "Network.h"
#include "TcpCore.h"

template<class S, class... Args>
template<class Socket>
struct Network<S, Args...>::TcpCore::TcpTx:
	IpTransmitterBase<TcpTx<Socket>>,
	TcpCore::InputChannel::WindowWaiter
{
	inline bool onSent(Launcher *launcher, IoJob* item, Result result) {
		state.increment(&DiagnosticCounters::Tcp::outputSent);

		// TODO put on retransmit
		return this->TcpTx::IpTransmitterBase::onSent(launcher, item, result);
	}

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload)
	{
		auto socket = this->getSocket();

	    switch(socket->state) {
            case TcpSocket::State::SynReceived:
            	socket->nextSequenceNumber += SeqNum(1);
                break;
            default:
            	socket->nextSequenceNumber += SeqNum(uint32_t(payload));
                break;
	    }

		Fixup::tcpTxPostProcess(stream, checksum, payload);
	}

	inline bool onPreparationDone(Launcher *launcher, IoJob* item)
	{
		using namespace TcpPacket;

		auto socket = this->getSocket();
        auto accessor = socket->getTxHeader();

	    switch(socket->state) {
            case TcpSocket::State::SynReceived:
                accessor.template get<Flags>().setDataOffset(20 + 4); // TODO mss option
                accessor.template get<Flags>().setSyn(true);
                accessor.template get<Flags>().setAck(true);
                break;
            default:
            	// TODO normal
                break;
	    }

        NET_ASSERT(accessor.fill(this->accessPacket()));

        NET_ASSERT(this->accessPacket().write8(2));
        NET_ASSERT(this->accessPacket().write8(4));
        NET_ASSERT(this->accessPacket().write16net(this->getSocket()->maximumTxSegmentSize));

		return TcpTx::IpTransmitterBase::IpTxJob::template startTransmission<InetChecksumDigester>(
				launcher, item, IpProtocolNumbers::tcp, 0x40 /* TODO */);
	}

	inline void sendSynAck(uint16_t mss = 536)
	{
		this->getSocket()->maximumRxSegmentSize = mss;
		const size_t size = 20 + 4;

		bool later = this->launch(
				&TcpTx::IpTransmitterBase::IpTxJob::startPreparation,
				this->getSocket()->accessTag().getPeerAddress(),
				size,
				0,
				arpRequestRetry);

        NET_ASSERT(later || this->getError() == nullptr);
	}

	inline void abandon() {
		this->cancel();
		this->wait();
	}
};

#endif /* TCPTX_H_ */
