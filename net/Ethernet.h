/*
 * Ethernet.h
 *
 *      Author: tamas.seller
 */

#ifndef ETHERNET_H_
#define ETHERNET_H_

template<class S, class... Args>
template<class Driver>
class Network<S, Args...>::Ethernet: public Os::template IoChannelBase<Ethernet<Driver>, Interface> {

    friend class Network<S, Args...>;

    ArpTable<Driver::arpCacheEntries> resolver;

    virtual const AddressEthernet& getAddress() override final {
        return Driver::ethernetAddress;
    }

    virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) override final {
        return resolver.lookUp(ip, mac);
    }

    virtual bool requestResolution(typename Os::IoJob::Hook hook, typename Os::IoJob* item, typename Os::IoJob::Callback callback, ArpTableIoData* data) override final {
        return item->submitTimeout(hook, &resolver, Driver::arpReqTimeout, callback, data);
    }

    virtual bool fillHeader(PacketBuilder& packet, const AddressEthernet& dst, uint16_t etherType) override final
    {
        if(packet.copyIn(reinterpret_cast<const char*>(dst.bytes), 6) != 6)
            return false;

        if(packet.copyIn(reinterpret_cast<const char*>(Driver::ethernetAddress.bytes), 6) != 6)
            return false;

        if(!packet.write16net(etherType))
            return false;

        return true;
    }

    void init() {
        Ethernet::IoChannelBase::init();
        resolver.init();
        Driver::init();
    }

public:

    inline Ethernet(): Ethernet::IoChannelBase(14) {}

    ArpTable<Driver::arpCacheEntries> *getArpCache() {
        return &resolver;
    }

    PacketTransmissionRequest *currentPacket, *nextPacket;
    pet::LinkedList<PacketTransmissionRequest> items;

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

    bool removeItem(typename Os::IoJob::Data* data) {
        Os::assert(data == currentPacket, "Invalid network packet queue state");
        currentPacket = nextPacket;
        auto it = items.iterator();

        if((nextPacket = it.current()) != nullptr)
            it.remove();

        return true;
    }

    bool removeCanceled(typename Os::IoJob::Data* data) {
        return this->items.remove(static_cast<PacketTransmissionRequest*>(data));
    }

    bool hasJob() {
        return currentPacket != nullptr;
    }

    inline PacketTransmissionRequest* getCurrentPacket() {
        return currentPacket;
    }

    inline PacketTransmissionRequest* getNextPacket() {
        return nextPacket;
    }

    void packetTransmitted() {
        this->jobDone(currentPacket);
    }
};

#endif /* ETHERNET_H_ */
