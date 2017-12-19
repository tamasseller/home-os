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
    typename Os::template Atomic<Block*> arpRequestQueue;

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

    inline void arpPacketReceived(Packet packet) {
    	this->putPacketChain(packet);
		static_cast<ArpReplyJob*>(this)->launch(&acquireBuffers);
    }

    /*
     * Internal
     */

	using IoJob = typename Os::IoJob;
	using Result = typename IoJob::Result;
	using Launcher = typename IoJob::Launcher;
	using Callback = typename IoJob::Callback;

    inline void putPacketChain(Block* first, Block *last) {
    	this->arpRequestQueue([](Block* old, Block*& result, Block* first, Block* last){
    		if(!old)
    			last->terminate();
			else
    			last->setNext(old);
			result = first;
			return true;
		}, first, last);
    }

    inline void putPacketChain(Packet packet) {
		Block* first = packet.first, *last = first;

		while(Block* next = last->getNext())
			last = next;

		this->putPacketChain(first, last);
    }

    inline bool takePacketFromQueue(Packet &ret) {
    	if(Block* first = this->arpRequestQueue.reset()) {
    		ret.init(first);

    		while(!first->isEndOfPacket()) {
    			first = first->getNext();
    			Os::assert(first, NetErrorStrings::unknown);
    		}

    		if(Block* next = first->getNext()) {
    			Packet remaining;
    			remaining.init(next);
    			this->putPacketChain(remaining);
    		}

    		first->terminate();
    		return true;
    	}

    	return false;
	}

    static inline bool replySent(Launcher *launcher, IoJob* item, Result result)
    {
    	Os::assert(result == Result::Done, NetErrorStrings::unknown);

    	auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));
    	self->txReq.template dispose<Pool::Quota::Tx>();

    	if(self->arpRequestQueue) {
    		acquireBuffers(launcher, item);
    		return true;
    	}

    	return false;
    }

	static inline bool buffersAcquired(Launcher *launcher, IoJob* item, Result result)
	{
		Os::assert(result == Result::Done, NetErrorStrings::unknown);

		auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));
		TxPacketBuilder builder;
		PacketStream reader;

		builder.init(self->poolParams.allocator);

		Packet requestPacket;
		bool takeOk = self->takePacketFromQueue(requestPacket);
		Os::assert(takeOk, NetErrorStrings::unknown);
		reader.init(requestPacket);

		/*
		 * Skip destination address of request (broadcast).
		 */
		bool skipOk = reader.skipAhead(6);
		Os::assert(skipOk, NetErrorStrings::unknown);

		/*
		 * Copy the source address of request (the destination of the reply).
		 */
		char requesterMac[6];
		bool dstReadOk = reader.copyOut(requesterMac, 6) == 6;
		Os::assert(dstReadOk, NetErrorStrings::unknown);

		bool dstWriteOk = builder.copyIn(requesterMac, 6) == 6;
		Os::assert(dstWriteOk, NetErrorStrings::unknown);

		/*
		 * Write source address (the MAC of the interface).
		 */
		bool srcWriteOk = builder.copyIn(reinterpret_cast<const char*>(self->resolver.getAddress().bytes), 6) == 6;
		Os::assert(srcWriteOk, NetErrorStrings::unknown);

		/*
		 * Write the boilerplate
		 */
		static constexpr const char replyBoilerplate[] = {
			0x08, 0x06,	// Ethertype
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
		AddressIp4 src = AddressIp4::make(0x0a, 0x0a, 0x0a, 0x0a);
		bool srcIpWriteOk = builder.write32net(src.addr);
		Os::assert(srcIpWriteOk, NetErrorStrings::unknown);

		/*
		 * Write destination address again.
		 */
		bool dstWriteAgainOk = builder.copyIn(requesterMac, 6) == 6;
		Os::assert(dstWriteAgainOk, NetErrorStrings::unknown);

		/*
		 * Skip middle of the packet all the way to the sender IP then read it.
		 */
		bool skipMiddleOk = reader.skipAhead(16);
		Os::assert(skipMiddleOk, NetErrorStrings::unknown);

		char requesterIp[4];
		bool dstIpReadOk = reader.copyOut(requesterIp, 4) == 4;
		Os::assert(dstIpReadOk, NetErrorStrings::unknown);

		/*
		 * Write destination IP address.
		 */
		bool dstIpWriteOk = builder.copyIn(requesterIp, 4) == 4;
		Os::assert(dstIpWriteOk, NetErrorStrings::unknown);

		builder.done();
		self->txReq.init(builder);

		bool ok = launcher->launch(self->getSender(), &Ethernet::replySent, &self->txReq);
		Os::assert(ok, NetErrorStrings::unknown);
		return true;
	}

	static constexpr uint16_t arpReplySize = 28;
	static constexpr uint16_t arpPacketBufferCount = (arpReplySize + /* + TODO l2 header + */ + blockMaxPayload - 1) / blockMaxPayload;

	static inline void initPoolParams(IoJob* item)
	{
		auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));
		self->poolParams.request.size = arpPacketBufferCount;
		self->poolParams.request.quota = Pool::Quota::Tx;
	}

    static inline bool acquireBuffers(Launcher *launcher, IoJob* item)
    {
		auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));

    	typename Pool::Allocator ret = state.pool.template allocateDirect<Pool::Quota::Rx>(arpPacketBufferCount);


    	if(!ret.hasMore())
       		return launcher->launch(&state.pool, &Ethernet::buffersAcquired, &self->poolParams, initPoolParams);

    	self->poolParams.allocator = ret; // TODO solve race with running job if not launched (direct allocation).

		return buffersAcquired(launcher, item, Result::Done);
    }

public:
    inline Ethernet(): Ethernet::IoChannelBase(14, &resolver) {}

    decltype(resolver) *getArpCache() {
        return &resolver;
    }
};

#endif /* ETHERNET_H_ */
