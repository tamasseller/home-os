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
#include "AddressIp4.h"
#include "AddressEthernet.h"
#include "NetErrorStrings.h"

struct NetworkOptions {
	PET_CONFIG_VALUE(RoutingTableEntries, size_t);
	PET_CONFIG_VALUE(ArpRequestRetry, size_t);
	PET_CONFIG_VALUE(BufferSize, size_t);
	PET_CONFIG_VALUE(BufferCount, size_t);
	PET_CONFIG_VALUE(MachineLittleEndian, bool);
	PET_CONFIG_TYPE(Interfaces);

	template<class... Members> struct Set {
		static constexpr size_t n = sizeof...(Members);
	};

	template<class Scheduler, class... Options>
	class Configurable {
		typedef Scheduler Os;
		PET_EXTRACT_VALUE(routingTableEntries, RoutingTableEntries, 4, Options);
		PET_EXTRACT_VALUE(arpRequestRetry, ArpRequestRetry, 3, Options);
		PET_EXTRACT_VALUE(swapBytes, MachineLittleEndian, true, Options);
		PET_EXTRACT_VALUE(bufferSize, BufferSize, 64, Options);
		PET_EXTRACT_VALUE(bufferCount, BufferCount, 64, Options);
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
			typedef BufferPool<OsRr, bufferCount, bufferSize> Pool;
			Pool pool;
		} state;

		class IpTxJob;

	public:
		typedef typename State::Pool::Storage Buffers;

		class Packet;
		class PacketWriter;
		class PacketReader;

		class IpTransmission;

		static inline constexpr uint32_t correctEndian(uint32_t);
		static inline constexpr uint16_t correctEndian(uint16_t);

		static inline void init(Buffers &buffers) {
		    state.~State();
		    new(&state) State();
			state.interfaces.init();
			state.pool(buffers);
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

#include "Packet.h"
#include "Endian.h"
#include "Routing.h"
#include "ArpTable.h"
#include "TxPacket.h"
#include "Interfaces.h"
#include "IpTransmission.h"

#endif /* NETWORK_H_ */
