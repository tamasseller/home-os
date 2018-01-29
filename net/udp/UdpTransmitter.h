/*
 * UdpTransmitter.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef UDPTRANSMITTER_H_
#define UDPTRANSMITTER_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::UdpTransmitter: public IpTransmitterBase<UdpTransmitter> {
	friend class UdpTransmitter::IpTransmitterBase::IpTxJob;

	uint16_t dstPort, srcPort;

	inline bool onSent(Launcher *launcher, IoJob* item, Result result) {
		state.increment(&DiagnosticCounters::Udp::outputSent);
		return this->UdpTransmitter::IpTransmitterBase::onSent(launcher, item, result);
	}

	static inline bool onPreparationDone(Launcher *launcher, IoJob* item) {
	    using namespace UdpPacket;
		auto self = static_cast<UdpTransmitter*>(item);

        StructuredAccessor<SourcePort, DestinationPort, End> accessor;
        accessor.get<SourcePort>() = self->srcPort;
        accessor.get<DestinationPort>() = self->dstPort;
        NET_ASSERT(accessor.fill(self->accessPacket()));

		return false;
	}

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payloadLength)
	{
	    using namespace UdpPacket;

		checksum.patch(correctEndian(static_cast<uint16_t>(IpProtocolNumbers::udp)));
		checksum.patch(correctEndian(static_cast<uint16_t>(payloadLength)));
		checksum.patch(correctEndian(static_cast<uint16_t>(payloadLength)));

        StructuredAccessor<Length, Checksum> accessor;
        accessor.get<Length>() = static_cast<uint16_t>(payloadLength);

        /*
         * RFC768 User Datagram Protocol (https://tools.ietf.org/html/rfc768) says:
         *
         *      "If the computed checksum is zero, it is transmitted as all
         *       ones (the equivalent in one's complement arithmetic). An all
         *       zero transmitted checksum value means that the transmitter
         *       generated  no checksum  (for debugging or for higher level
         *       protocols that don't care)."
         */
        accessor.get<Checksum>() = checksum.result() ? checksum.result() : 0xffff;

        NET_ASSERT(accessor.fill(stream));

		state.increment(&DiagnosticCounters::Udp::outputQueued);
	}
public:

	inline void setSourcePort(uint16_t srcPort) {
		this->srcPort = srcPort;
	}

	inline void init(uint16_t srcPort) {
		this->srcPort = srcPort;
		UdpTransmitter::IpTransmitterBase::init();
	}

	inline bool prepare(AddressIp4 dst, uint16_t dstPort, size_t inLineSize, size_t indirectCount = 0)
	{
    	this->reset();
    	inLineSize += 8; // UDP header size;
    	this->dstPort = dstPort;
		bool later = this->launch(&UdpTransmitter::IpTransmitterBase::IpTxJob::startPreparation, dst, inLineSize, indirectCount, arpRequestRetry);
		return later || this->getError() == nullptr;
	}

    inline bool prepareTimeout(size_t timeout, AddressIp4 dst, size_t inLineSize, size_t indirectCount = 0)
    {
    	this->reset();
    	inLineSize += 8; // UDP header size;
    	this->dstPort = dstPort;
        bool later = this->launchTimeout(&UdpTransmitter::IpTransmitterBase::IpTxJob::startPreparation, timeout, dst, inLineSize, indirectCount, arpRequestRetry);
        return later || this->getError() == nullptr;
    }

	inline bool send(uint8_t ttl = 64)
	{
		if(!this->check())
			return false;

		bool later = this->launch(
				&UdpTransmitter::IpTransmitterBase::IpTxJob::template startTransmission<InetChecksumDigester>,
				IpProtocolNumbers::udp, ttl);
        return later || this->getError() == nullptr;
	}

    inline bool sendTimeout(size_t timeout, uint8_t ttl = 64)
    {
        if(!this->check())
            return false;

        bool later = this->launchTimeout(
        		&UdpTransmitter::IpTransmitterBase::IpTxJob::template startTransmission<InetChecksumDigester>, timeout,
        		IpProtocolNumbers::udp, ttl);
        return later || this->getError() == nullptr;
    }
};

#endif /* UDPTRANSMITTER_H_ */
