/*
 * Ethernet.h
 *
 *      Author: tamas.seller
 */

#ifndef ETHERNET_H_
#define ETHERNET_H_

template<class S, class... Args>
template<class Driver>
class Network<S, Args...>::Ethernet:
	public Os::template IoChannelBase<Ethernet<Driver>, Interface>,
	public ArpCore<Driver>,
	public Driver
{
    friend class Ethernet::IoChannelBase;
    friend class Network<S, Args...>;
    friend Driver;

    PacketTransmissionRequest *currentPacket, *nextPacket;
    pet::LinkedList<PacketTransmissionRequest> items;

    /*
     * Interface towards the network interface (called _Interface_ :) )
     */
    inline void init() {
        Ethernet::IoChannelBase::init();
        Ethernet::ArpCore::init();
        Driver::init();
    }

    inline void ageContent() {
    	Ethernet::ArpCore::ageContent();
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

public:
    inline Ethernet():
    	Ethernet::IoChannelBase((uint16_t)14, &this->Ethernet::ArpCore::resolver) {}

    typename Ethernet::ArpCore::Resolver *getArpCache() {
        return &this->Ethernet::ArpCore::resolver;
    }
};

#endif /* ETHERNET_H_ */
