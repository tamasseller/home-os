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
	public ArpReplyJob,	// TODO move into ArpCore
	private PacketProcessor, // TODO move into ArpCore
	public Driver
{
    friend class Ethernet::IoChannelBase;
    friend class Network<S, Args...>;
    friend Driver;

    using PacketBuilder = Network<S, Args...>::PacketBuilder;
    struct Resolver;

    Resolver resolver;
    PacketTransmissionRequest *currentPacket, *nextPacket;
    pet::LinkedList<PacketTransmissionRequest> items;
    PacketChain arpRequestQueue;
    union {
    	typename Pool::IoData poolParams;
    	PacketTransmissionRequest txReq;
    };

    /*
     * Interface towards the network interface (called _Interface_ :) )
     */
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

    /*
     * Implementation of the TX _IoChannel_.
     */
    void enableProcess() {
    	Driver::enableTxIrq();
    }

    void disableProcess() {
    	Driver::disableTxIrq();
    }

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

    inline void arpPacketReceived(Packet packet);

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

    /*
     * Internal
     */
	using IoJob = typename Os::IoJob;
	using Result = typename IoJob::Result;
	using Launcher = typename IoJob::Launcher;
	using Callback = typename IoJob::Callback;

	inline void processArpReplyPacket(Packet chain); // TODO move into ArpCore::ArpReplyJob
    static inline bool replySent(Launcher *launcher, IoJob* item, Result result); // TODO move into ArpCore::ArpReplyJob
	static inline bool assembleReply(Launcher *launcher, IoJob* item, Result result); // TODO move into ArpCore::ArpReplyJob
    static inline bool startReplySequence(Launcher *launcher, IoJob* item); // TODO move into ArpCore::ArpReplyJob

public:
    inline Ethernet():
    	Ethernet::IoChannelBase((uint16_t)14, &resolver),
    	PacketProcessor(PacketProcessor::template make<Ethernet, &Ethernet::processArpReplyPacket>()) {}

    decltype(resolver) *getArpCache() {
        return &resolver;
    }
};

#endif /* ETHERNET_H_ */
