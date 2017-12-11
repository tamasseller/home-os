/*
 * Ethernet.h
 *
 *      Author: tamas.seller
 */

#ifndef ETHERNET_H_
#define ETHERNET_H_

template<class S, class... Args>
template<class Driver>
class Network<S, Args...>::Ethernet: public Os::template IoChannelBase<Ethernet<Driver>, Interface>, public Driver
{
    friend class Ethernet::IoChannelBase;
    friend class Network<S, Args...>;
    friend Driver;

    struct Resolver: ArpTable<Os, Driver::arpCacheEntries, typename Interface::AddressResolver> {
        virtual const AddressEthernet& getAddress() override final {
            return Driver::ethernetAddress;
        }

        virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) override final {
            return this->lookUp(ip, mac);
        }

		virtual bool fillHeader(TxPacketBuilder& packet, const AddressEthernet& dst, uint16_t etherType) override final
		{
			if(packet.copyIn(reinterpret_cast<const char*>(dst.bytes), 6) != 6)
				return false;

			if(packet.copyIn(reinterpret_cast<const char*>(Driver::ethernetAddress.bytes), 6) != 6)
				return false;

			if(!packet.write16net(etherType))
				return false;

			return true;
		}
    } resolver;

    inline void init() {
        Ethernet::IoChannelBase::init();
        resolver.init();
        Driver::init();
    }

    inline void ageContent() {
    	resolver.ageContent();
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
    inline Ethernet(): Ethernet::IoChannelBase(14, &resolver) {}

    decltype(resolver) *getArpCache() {
        return &resolver;
    }
};

#endif /* ETHERNET_H_ */
