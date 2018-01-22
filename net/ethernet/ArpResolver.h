/*
 * ArpResolver.h
 *
 *  Created on: 2018.01.22.
 *      Author: tooma
 */

#ifndef ARPRESOLVER_H_
#define ARPRESOLVER_H_

#include "Network.h"

template<class S, class... Args>
template<class Driver>
struct Network<S, Args...>::ArpCore<Driver>::Resolver:
	ArpTable<Os, Driver::arpCacheEntries, typename Interface::LinkAddressingManager>
{
    template<class T> static void callPre(PacketBuilder& packet, decltype(T::preHeaderFill(packet))*) { return T::preHeaderFill(packet); }
    template<class T> static void callPre(...) {}

	virtual const AddressEthernet& getAddress() override final {
		return Driver::ethernetAddress;
	}

	virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) override final {
		return this->lookUp(ip, mac);
	}

	virtual void fillHeader(PacketBuilder& packet, const AddressEthernet& dst, uint16_t etherType) override final
	{
		callPre<Driver>(packet, 0);

		NET_ASSERT(packet.copyIn(reinterpret_cast<const char*>(dst.bytes), 6) == 6);
		NET_ASSERT(packet.copyIn(reinterpret_cast<const char*>(Driver::ethernetAddress.bytes), 6) == 6);
		NET_ASSERT(packet.write16net(etherType));
	}
};

#endif /* ARPRESOLVER_H_ */
