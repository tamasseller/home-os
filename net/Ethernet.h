/*
 * Ethernet.h
 *
 *      Author: tamas.seller
 */

#ifndef ETHERNET_H_
#define ETHERNET_H_

template<class S, class... Args>
struct Network<S, Args...>::ArpReplyJob: public Os::IoJob {};

template<class S, class... Args>
template<class Driver>
class Network<S, Args...>::Ethernet:
	public Os::template IoChannelBase<Ethernet<Driver>, Interface>,
	public ArpReplyJob,
	private PacketProcessor,
	public Driver
{
    friend class Ethernet::IoChannelBase;
    friend class Network<S, Args...>;
    friend Driver;

    using TxPacketBuilder = Network<S, Args...>::TxPacketBuilder;

    template<class T> static void callPre(TxPacketBuilder& packet, decltype(T::preHeaderFill(packet))*) { return T::preHeaderFill(packet); }
    template<class T> static void callPre(...) {}

    struct Resolver: ArpTable<Os, Driver::arpCacheEntries, typename Interface::AddressResolver> {
        virtual const AddressEthernet& getAddress() override final {
            return Driver::ethernetAddress;
        }

        virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) override final {
            return this->lookUp(ip, mac);
        }

		virtual void fillHeader(TxPacketBuilder& packet, const AddressEthernet& dst, uint16_t etherType) override final
		{
			callPre<Driver>(packet, 0);

			bool dstOk = packet.copyIn(reinterpret_cast<const char*>(dst.bytes), 6) == 6;
			Os::assert(dstOk, NetErrorStrings::unknown);
			bool srcOk = packet.copyIn(reinterpret_cast<const char*>(Driver::ethernetAddress.bytes), 6) == 6;
			Os::assert(srcOk, NetErrorStrings::unknown);
			bool typeOk = packet.write16net(etherType);
			Os::assert(typeOk, NetErrorStrings::unknown);
		}
    } resolver;

    PacketTransmissionRequest *currentPacket, *nextPacket;
    pet::LinkedList<PacketTransmissionRequest> items;
    PacketQueue arpRequestQueue;

    union {
    	typename Pool::IoData poolParams;
    	PacketTransmissionRequest txReq;
    };


    inline void init() {
        Ethernet::IoChannelBase::init();
        resolver.init();
        Driver::init();
    }

    inline void ageContent() {
    	resolver.ageContent();
    }

    virtual void takeRxBuffers(typename Pool::Allocator allocator) override final {
    	while(allocator.hasMore()) {
    		if(!this->Driver::takeRxBuffer(allocator.get()->getData()))
    			break; // Interface should take what it asked for. (LCOV_EXCL_LINE)
    	}

    	allocator.template freeSpare<Pool::Quota::Rx>(&state.pool);
    }

    void enableProcess() {Driver::enableTxIrq();}
    void disableProcess() {Driver::disableTxIrq();}

    bool addItem(typename Os::IoJob::Data* data)
    {
        auto* p = static_cast<PacketTransmissionRequest*>(data);

        if(!currentPacket)
            currentPacket = p;
        else if(!nextPacket)
            nextPacket = p;
        else
            return this->items.addBack(p);

        return true;
    }

    bool removeCanceled(typename Os::IoJob::Data* data) {
    	auto* p = static_cast<PacketTransmissionRequest*>(data);

    	if(p == currentPacket || p == nextPacket)
    		return false;

        this->items.remove(static_cast<PacketTransmissionRequest*>(data));
        return true;
    }

    bool hasJob() {
        return currentPacket != nullptr;
    }

    /*
     * Interface towards the driver.
     */

    inline PacketTransmissionRequest* getCurrentTxPacket() {
        return currentPacket;
    }

    inline PacketTransmissionRequest* getNextTxPacket() {
        return nextPacket;
    }

    void packetTransmitted() {
    	auto* p = currentPacket;

        currentPacket = nextPacket;
        auto it = items.iterator();

        if((nextPacket = it.current()) != nullptr)
            it.remove();

        this->jobDone(p);
    }

    inline void arpPacketReceived(Packet packet)
    {
		PacketStream reader;
		reader.init(packet);
		reader.skipAhead(static_cast<Interface*>(this)->getHeaderSize()); // TODO error handling
		uint16_t hType = reader.read16net();
		uint16_t pType = reader.read16net();
		uint8_t hLen = reader.read8();
		uint8_t pLen = reader.read8();

		if(hType != 1 || hLen != 6 || pType != 0x800 || pLen != 4) {
			packet.template dispose<Pool::Quota::Rx>();
		} else {
			uint16_t opCode = reader.read16net();

			if(opCode == 0x01) {
		    	this->arpRequestQueue.putPacketChain(packet);
				static_cast<ArpReplyJob*>(this)->launch(&acquireBuffers);
			} else if(opCode == 0x0002) {
				static_cast<PacketProcessor*>(this)->process(packet);
			} else
				packet.template dispose<Pool::Quota::Rx>();
		}
    }

    /*
     * Internal
     */

	using IoJob = typename Os::IoJob;
	using Result = typename IoJob::Result;
	using Launcher = typename IoJob::Launcher;
	using Callback = typename IoJob::Callback;

	inline void processArpReplyPacket(PacketStream reader)
	{
		/*
		 * Save the handle to the first block for later disposal.
		 */
		typename PacketDisassembler::Cursor start = reader.getCursor();

		/*
		 * Skip destination ethernet header and initial fields of the ARP payload all the
		 * way to the sender hardware address, the initial fields are already processed at
		 * this point and are known to describe an adequate reply message.
		 */
		bool skipOk = reader.skipAhead(static_cast<uint16_t>(this->getHeaderSize() + 8));
		Os::assert(skipOk, NetErrorStrings::unknown);

		AddressEthernet replyMac;
		bool macReadOk = reader.copyOut(reinterpret_cast<char*>(replyMac.bytes), 6) == 6;
		Os::assert(macReadOk, NetErrorStrings::unknown);

		AddressIp4 replyIp;
		replyIp.addr = reader.read32net(); // TODO handle error

		/*
		 * Move reader to the end, then dispose of the reply packet.
		 */
//		reader.skipToEnd();
//		reader.template dropFrom<Pool::Quota::Rx>(start);

		/*
		 * Notify the ARP table about the resolved address.
		 */
		resolver.handleResolved(replyIp, replyMac, arpTimeout);
	}

    static inline bool replySent(Launcher *launcher, IoJob* item, Result result)
    {
    	Os::assert(result == Result::Done, NetErrorStrings::unknown);

    	auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));
    	self->txReq.template dispose<Pool::Quota::Tx>();

    	if(!self->arpRequestQueue.isEmpty()) {
    		acquireBuffers(launcher, item);
    		return true;
    	}

    	return false;
    }

	static inline bool buffersAcquired(Launcher *launcher, IoJob* item, Result result)
	{
		bool ret = false;
		Os::assert(result == Result::Done, NetErrorStrings::unknown);

		auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));
		TxPacketBuilder builder;

		builder.init(self->poolParams.allocator);

		Packet requestPacket;
		while(self->arpRequestQueue.takePacketFromQueue(requestPacket)) {
			PacketStream reader;
			reader.init(requestPacket);

			/*
			 * Skip destination ethernet header and initial fields of the ARP payload all the
			 * way to the sender hardware address, the initial fields are already processed at
			 * this point and are known to describe an adequate request.
			 */
			bool skipOk = reader.skipAhead(static_cast<uint16_t>(self->getHeaderSize() + 8));
			Os::assert(skipOk, NetErrorStrings::unknown);

			/*
			 * Save the source address of the request for later use as the destination of the reply.
			 */
			AddressEthernet requesterMac;
			bool dstReadOk = reader.copyOut(reinterpret_cast<char*>(requesterMac.bytes), 6) == 6;
			Os::assert(dstReadOk, NetErrorStrings::unknown);

			/*
			 * Read sender IP address.
			 */
			char requesterIp[4];
			bool dstIpReadOk = reader.copyOut(requesterIp, 4) == 4;
			Os::assert(dstIpReadOk, NetErrorStrings::unknown);

			bool skip2Ok = reader.skipAhead(static_cast<uint16_t>(6));
			Os::assert(skip2Ok, NetErrorStrings::unknown);

			/*
			 * Read sender IP address.
			 */
			AddressIp4 queriedIp;
			queriedIp.addr = reader.read32net(); // TODO error handling

			/*
			 * Check if the requested
			 */
			Route* route = state.routingTable.findRouteWithSource(self, queriedIp);
			if(!route) {
				requestPacket.template dispose<Pool::Quota::Rx>();
				continue;
			} else
				state.routingTable.releaseRoute(route);

			/*
			 * Fill ethernet header with the requester as the destination and write the ARP boilerplate.
			 */
			self->resolver.fillHeader(builder, requesterMac, 0x0806);

			static constexpr const char replyBoilerplate[] = {
				0x00, 0x01, // hwType
				0x08, 0x00, // protoType
				0x06,		// hSize
				0x04, 		// pSize
				0x00, 0x02	// opCode
			};

			bool boilerplateOk = builder.copyIn(replyBoilerplate, sizeof(replyBoilerplate)) == sizeof(replyBoilerplate);
			Os::assert(boilerplateOk, NetErrorStrings::unknown);

			/*
			 * Write source address again.
			 */
			bool srcWriteAgainOk = builder.copyIn(reinterpret_cast<const char*>(self->resolver.getAddress().bytes), 6) == 6;
			Os::assert(srcWriteAgainOk, NetErrorStrings::unknown);

			/*
			 * Write source IP address.
			 */
			bool srcIpWriteOk = builder.write32net(queriedIp.addr);
			Os::assert(srcIpWriteOk, NetErrorStrings::unknown);

			/*
			 * Write destination address again.
			 */
			bool dstWriteAgainOk = builder.copyIn(reinterpret_cast<char*>(requesterMac.bytes), 6) == 6;
			Os::assert(dstWriteAgainOk, NetErrorStrings::unknown);

			/*
			 * Write destination IP address.
			 */
			bool dstIpWriteOk = builder.copyIn(requesterIp, 4) == 4;
			Os::assert(dstIpWriteOk, NetErrorStrings::unknown);

			requestPacket.template dispose<Pool::Quota::Rx>();
			ret = true;
			break;
		}

		if(ret) {
			builder.done();
			self->txReq.init(builder);
			launcher->launch(self->getSender(), &Ethernet::replySent, &self->txReq);
		} else {
			builder.done();
			builder.template dispose<Pool::Quota::Tx>();
		}

		return true;
	}

    static inline bool acquireBuffers(Launcher *launcher, IoJob* item)
    {
    	static constexpr uint16_t arpReplySize = 28;
    	static constexpr uint16_t bufferCount = (arpReplySize + /* + TODO l2 header + */ + blockMaxPayload - 1) / blockMaxPayload;

    	return state.pool.template allocateDirectOrDeferred<Pool::Quota::Tx>(
    			launcher,
    			&Ethernet::buffersAcquired,
    			&static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item))->poolParams,
    			bufferCount);
    }

public:
    inline Ethernet():
    	Ethernet::IoChannelBase((uint16_t)14, &resolver),
    	PacketProcessor(PacketProcessor::template make<Ethernet, &Ethernet::processArpReplyPacket>()) {}

    decltype(resolver) *getArpCache() {
        return &resolver;
    }
};

#endif /* ETHERNET_H_ */
