/*
 * Network.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "meta/Configuration.h"

#include "Routing.h"
#include "ArpTable.h"
#include "AddressIp4.h"
#include "BufferPool.h"
#include "SharedTable.h"
#include "AddressEthernet.h"
#include "NetErrorStrings.h"
#include "InetChecksumDigester.h"

struct NetworkOptions {
	PET_CONFIG_VALUE(RoutingTableEntries, size_t);
	PET_CONFIG_VALUE(ArpRequestRetry, uint8_t);
	PET_CONFIG_VALUE(ArpCacheTimeout, size_t);
	PET_CONFIG_VALUE(BufferSize, size_t);
	PET_CONFIG_VALUE(BufferCount, size_t);
	PET_CONFIG_VALUE(TxBufferLimit, size_t);
	PET_CONFIG_VALUE(RxBufferLimit, size_t);
	PET_CONFIG_VALUE(TicksPerSecond, size_t);
	PET_CONFIG_VALUE(MachineLittleEndian, bool);
	PET_CONFIG_VALUE(ArpAntiSpoof, bool);
	PET_CONFIG_TYPE(Interfaces);

	template<template<class, uint16_t> class Driver> struct EthernetInterface {
	        template<class Net, uint16_t n> using Wrapped = typename Net::template Ethernet<Driver<Net, n>>;
	};

	template<class... Members> struct Set {
		static constexpr size_t n = sizeof...(Members);
	};

	template<class Scheduler, class... Options>
	class Configurable {
		template<class> friend class NetworkTestAccessor;
		friend class NetworkOptions;

		PET_EXTRACT_VALUE(routingTableEntries, RoutingTableEntries, 4, Options);
		PET_EXTRACT_VALUE(arpRequestRetry, ArpRequestRetry, 3, Options);
		PET_EXTRACT_VALUE(swapBytes, MachineLittleEndian, true, Options);
		PET_EXTRACT_VALUE(arpAntiSpoof, ArpAntiSpoof, false, Options);
		PET_EXTRACT_VALUE(bufferSize, BufferSize, 64, Options);
		PET_EXTRACT_VALUE(bufferCount, BufferCount, 64, Options);
		PET_EXTRACT_VALUE(arpTimeout, ArpCacheTimeout, 60, Options);
		PET_EXTRACT_VALUE(txBufferLimit, TxBufferLimit, bufferCount * 75 / 100, Options);
		PET_EXTRACT_VALUE(rxBufferLimit, RxBufferLimit, bufferCount * 75 / 100, Options);

		PET_CONFIG_VALUE(TxBufferLimit, size_t);
		PET_CONFIG_VALUE(RxBufferLimit, size_t);
		PET_EXTRACT_VALUE(secTicks, TicksPerSecond, 1000, Options);
		PET_EXTRACT_TYPE(IfsToBeUsed, Interfaces, Set<>, Options);

		static_assert(IfsToBeUsed::n, "No interfaces specified");
		static_assert(bufferCount < 32768, "Too many blocks requested");

	public:
		class Interface;
		class Packet;
		class PacketStream;
		class PacketAssembler;
		class PacketDisassembler;
		typedef Scheduler Os;

	private:
		template<class> class Ethernet;
		template<uint16_t, class, class = void> struct Interfaces;

		struct Chunk;
		class Block;
		class PacketQueue;

		class NullTag;
		class DstPortTag;

		template<class> class PacketInputChannel;
		typedef PacketInputChannel<NullTag> IcmpInputChannel;
		typedef PacketInputChannel<NullTag> RawInputChannel;
		typedef PacketInputChannel<DstPortTag> UdpInputChannel;

		template<class> class PacketWriterBase;
		class NullObserver;
		template<class> class ObservedPacketStream;
		class PacketTransmissionRequest;

		typedef BufferPool<Os, bufferCount, Block, txBufferLimit, rxBufferLimit> Pool;
		typedef ::RoutingTable<Os, Interface, routingTableEntries> RoutingTable;
		template<typename Pool::Quota> class PacketBuilder;
		using TxPacketBuilder = PacketBuilder<Pool::Quota::Tx>;
		using RxPacketBuilder = PacketBuilder<Pool::Quota::Rx>;

		class DummyDigester;

		class RxPacketHandler;
		template<class> class IpTxJob;
		template<class, class> class IpRxJob;
		template<class> class IpReplyJob;
		class IcmpEchoReplyJob;
		class ArpReplyJob;

		class PacketProcessor;
		class ArpPacketProcessor;
		class IcmpPacketProcessor;
		class RawPacketProcessor;
		class UdpPacketProcessor;

		static struct State {
			struct Ager: Os::Timeout {
				inline void age();
				static inline void doAgeing(typename Os::Sleeper*);
				inline Ager(): Os::Timeout(&Ager::doAgeing) {}
			} ager;

            inline void* operator new(size_t, void* x) { return x; }

            IcmpEchoReplyJob icmpReplyJob;
			IcmpPacketProcessor icmpPacketProcessor;
			IcmpInputChannel icmpInputChannel;

			RawInputChannel rawInputChannel;
			RawPacketProcessor rawPacketProcessor;

			UdpInputChannel udpInputChannel;
			UdpPacketProcessor udpPacketProcessor;

			Interfaces<Block::dataSize, IfsToBeUsed> interfaces;
			RoutingTable routingTable;
			Pool pool;
		} state;

		static void fillInitialIpHeader(TxPacketBuilder &packet, AddressIp4 srcIp, AddressIp4 dstIp);

		template<bool patch, class HeaderDigester, class PayloadDigester>
		static inline uint16_t headerFixupStepOne(
				Packet packet,
				size_t l2headerLength,
				size_t ipHeaderLength,
				HeaderDigester &headerChecksum,
				PayloadDigester &payloadChecksum,
				uint8_t ttl,
				uint8_t protocol);

		static inline void headerFixupStepTwo(
				PacketStream &modifier,
				size_t l2HeaderSize,
				uint16_t length,
				uint16_t headerChecksum);

		inline static constexpr uint16_t bytesToBlocks(size_t);

		using IoJob = typename Os::IoJob;
		using Result = typename IoJob::Result;
		using Launcher = typename IoJob::Launcher;
		using Callback = typename IoJob::Callback;

		template<class Reader> static inline RxPacketHandler* checkIcmpPacket(Reader&);
		template<class Reader> static inline RxPacketHandler* checkUdpPacket(Reader&);
		template<class Reader> static inline RxPacketHandler* checkTcpPacket(Reader&);
		static inline void processIcmpPacket(typename Os::Event*, uintptr_t);
		static inline void processRawPacket(typename Os::Event*, uintptr_t);
		static inline void processUdpPacket(typename Os::Event*, uintptr_t);
		static inline void ipPacketReceived(Packet packet, Interface* dev);

	public:
		static constexpr auto blockMaxPayload = Block::dataSize;
		typedef typename Pool::Storage Buffers;
		using Route = typename RoutingTable::Route;

		class IpTransmitter;
		class IpReceiver;
		class IcmpReceiver;
		class UdpReceiver;

		static inline constexpr uint32_t correctEndian(uint32_t);
		static inline constexpr uint16_t correctEndian(uint16_t);

		static inline void init(Buffers &buffers) {
		    state.~State();
		    new(&state) State();
		    state.icmpInputChannel.init();
		    state.rawInputChannel.init();
		    state.udpInputChannel.init();
			state.pool.init(buffers);
			state.interfaces.init();
			state.ager.start(secTicks);
		}

		static inline bool addRoute(const Route& route, bool setUp = false) {
		    return state.routingTable.add(route, setUp);
		}

		template<template<class, uint16_t> class Driver>
		constexpr static inline auto* getEthernetInterface();
	};
};

template<class S, class... Args>
using Network = NetworkOptions::Configurable<S, Args...>;

template<class S, class... Args>
typename Network<S, Args...>::State NetworkOptions::Configurable<S, Args...>::state;

template<class S, class... Args>
inline void Network<S, Args...>::State::Ager::age() {
	state.interfaces.ageContent();
	this->extend(secTicks);
}

template<class S, class... Args>
inline void Network<S, Args...>::State::Ager::doAgeing(typename Os::Sleeper* sleeper) {
	static_cast<Ager*>(sleeper)->age();
}

template<class S, class... Args>
inline constexpr uint16_t Network<S, Args...>::bytesToBlocks(size_t bytes) {
	return static_cast<uint16_t>((bytes + blockMaxPayload - 1) / blockMaxPayload);
}

template<class S, class... Args>
struct Network<S, Args...>::RxPacketHandler {
	virtual void handlePacket(Packet packet) = 0;
};

#include "Arp.h"
#include "Udp.h"
#include "Icmp.h"
#include "Packet.h"
#include "Endian.h"
#include "Ethernet.h"
#include "Interfaces.h"
#include "IpReplyJob.h"
#include "IpReception.h"
#include "PacketQueue.h"
#include "PacketStream.h"
#include "PacketBuilder.h"
#include "IpTransmission.h"
#include "PacketProcessor.h"
#include "PacketInputChannel.h"

#endif /* NETWORK_H_ */
