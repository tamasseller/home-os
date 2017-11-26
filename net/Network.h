/*
 * Network.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "meta/Configuration.h"

#include "BufferPool.h"
#include "SharedTable.h"
#include "Packet.h"
#include "AddressIp4.h"
#include "AddressEthernet.h"
#include "NetErrorStrings.h"

struct NetworkOptions {
	PET_CONFIG_VALUE(RoutingTableEntries, size_t);
	PET_CONFIG_VALUE(ArpRequestRetry, size_t);
	PET_CONFIG_TYPE(Interfaces);

	template<class... Members> struct Set {
		static constexpr size_t n = sizeof...(Members);
	};

	template<class Scheduler, class... Options>
	class Configurable {
		typedef Scheduler Os;
		PET_EXTRACT_VALUE(routingTableEntries, RoutingTableEntries, 4, Options);
		PET_EXTRACT_VALUE(arpRequestRetry, ArpRequestRetry, 3, Options);
		PET_EXTRACT_TYPE(IfsToBeUsed, Interfaces, Set<>, Options);

		static_assert(IfsToBeUsed::n, "No interfaces specified");

	public:
		class Interface;
		class Route;

	private:
		template<class> class TxBinder;
		template<class, class = void> struct Ifs;

		class ArpEntry;
		class ArpTableIoData;
		template<size_t> class ArpTable;

		class RoutingTable;
		class TxPacket;

		static struct State {
			Ifs<IfsToBeUsed> interfaces;
			RoutingTable routingTable;
		} state;

		class IpTxJob;

	public:

		class IpTransmission;

		static inline void init() {
			state.interfaces.init();
		}

		static inline bool addRoute(const Route& route) {
		    return state.routingTable.add(route);
		}

		template<class C> constexpr static inline TxBinder<C> *getIf();
	};
};

template<class S, class... Args>
typename NetworkOptions::Configurable<S, Args...>::State NetworkOptions::Configurable<S, Args...>::state;

template<class S, class... Args>
using Network = NetworkOptions::Configurable<S, Args...>;

#include "ArpTable.h"
#include "Interfaces.h"
#include "IpTransmission.h"
#include "Routing.h"
#include "TxPacket.h"

#endif /* NETWORK_H_ */
