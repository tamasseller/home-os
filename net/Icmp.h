/*
 * Icmp.h
 *
 *  Created on: 2017.12.21.
 *      Author: tooma
 */

#ifndef ICMP_H_
#define ICMP_H_

#include "Network.h"

template<class S, class... Args>
inline void Network<S, Args...>::processIcmpPacket(typename Os::Event*, uintptr_t arg)
{
	Packet chain;
	chain.init(reinterpret_cast<Block*>(arg));
}

template<class S, class... Args>
class Network<S, Args...>::IcmpReplyJob: IpTxJob<IcmpReplyJob>
{
	friend class IcmpReplyJob::IpTxJob;

    PacketQueue requestQueue;
    Packet request;

    inline bool restartIfNeeded(Launcher *launcher, IoJob* item)
    {
        if(!this->requestQueue.isEmpty()) {
            startReplySequence(launcher, item);
            return true;
        }

        return false;
    }

    inline bool onPrepFailed(Launcher *launcher, IoJob* item) {
        this->request.template dispose<Pool::Quota::Rx>();
        return restartIfNeeded(launcher, item);
    }

    inline bool onSent(Launcher *launcher, IoJob* item, Result result) {
    	this->packet.stage3.template dispose<Pool::Quota::Tx>();
    	return restartIfNeeded(launcher, item);
    }

	inline bool onPreparationDone(Launcher *launcher, IoJob* item)
	{
		PacketStream reader;
		reader.init(request);

    	uint8_t headerLength;
    	reader.read8(headerLength);
    	headerLength = static_cast<uint8_t>((headerLength & 0xf) << 2);
    	reader.skipAhead(static_cast<uint16_t>(headerLength - 1));

		uint16_t typeCode;
		reader.read16net(typeCode);

		if(typeCode == 0x800) {
			uint16_t checksum;
			reader.read16net(checksum);
			uint32_t temp = checksum + 0x800;
			temp = (temp & 0xffff) + (temp >> 16);
			checksum = static_cast<uint16_t>((temp & 0xffff) + (temp >> 16));

			this->packet.stage2.write16net(0); // type and code
			this->packet.stage2.write16net(checksum); // type and code

			do {
				Chunk chunk = reader.getChunk();

				bool ok = this->packet.stage2.copyIn(chunk.start, static_cast<uint16_t>(chunk.length()));
				Os::assert(ok, NetErrorStrings::unknown);

				reader.advance(static_cast<uint16_t>(chunk.length()));
			} while(!reader.atEop());
		}

		request.template dispose<Pool::Quota::Rx>();

		return IcmpReplyJob::IpTxJob::startTransmission(launcher, item, 0x01 /* ICMP */, 0xff);
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

    inline bool onTimeout(Launcher *launcher, IoJob* item) {
        Os::assert(false, NetErrorStrings::unknown);
        return false;
    }

    inline bool onCancel(Launcher *launcher, IoJob* item) {
        Os::assert(false, NetErrorStrings::unknown);
        return false;
    }

    /*
     * These methods should never be called. (LCOV_EXCL_STOP)
     */

    static bool startReplySequence(Launcher *launcher, IoJob* item) {
    	auto self = static_cast<IcmpReplyJob*>(item);

    	self->requestQueue.takePacketFromQueue(self->request);

		PacketStream reader;
		reader.init(self->request);

    	uint8_t headerLength;
    	reader.read8(headerLength);
    	headerLength = static_cast<uint8_t>((headerLength & 0xf) << 2);

		reader.skipAhead(1);	// skip version+ihl and dsp+ecn

		uint16_t totalLength;
		reader.read16net(totalLength);

		reader.skipAhead(8);

		AddressIp4 requesterIp;
		reader.read32net(requesterIp.addr);

    	// TODO check ICMP sum. (type and code is known to be ok)

		return IcmpReplyJob::IpTxJob::startPreparation(launcher, item, requesterIp, totalLength - headerLength, 0, 0);
    }

public:
    void arrangeReply(Packet packet) {
    	this->requestQueue.putPacketChain(packet);
		this->launch(&IcmpReplyJob::startReplySequence);
    }
};

#endif /* ICMP_H_ */
