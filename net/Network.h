/*
 * Network.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "core/SeqNum.h"
#include "core/Routing.h"
#include "core/ArpTable.h"
#include "core/AddressIp4.h"
#include "core/BufferPool.h"
#include "core/Initializer.h"
#include "core/SharedTable.h"
#include "core/AddressEthernet.h"
#include "core/NetErrorStrings.h"
#include "core/IpProtocolNumbers.h"
#include "core/DiagnosticCounters.h"
#include "core/InetChecksumDigester.h"

#include "meta/Sfinae.h"
#include "meta/Configuration.h"

struct NetworkOptions {
	PET_CONFIG_VALUE(RoutingTableEntries, size_t);
	PET_CONFIG_VALUE(ArpRequestRetry, uint8_t);
	PET_CONFIG_VALUE(ArpCacheTimeout, size_t);
	PET_CONFIG_VALUE(BufferSize, size_t);
	PET_CONFIG_VALUE(BufferCount, size_t);
	PET_CONFIG_VALUE(TxBufferLimit, size_t);
	PET_CONFIG_VALUE(RxBufferLimit, size_t);
	PET_CONFIG_VALUE(TicksPerSecond, size_t);
	PET_CONFIG_VALUE(UseDiagnosticCounters, bool);
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
		PET_EXTRACT_VALUE(useDiagnosticCounters, UseDiagnosticCounters, false, Options);
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
		typedef Scheduler Os;
		class Interface;
		class Packet;

	private:
		using IoJob = typename Os::IoJob;
		using Result = typename IoJob::Result;
		using Launcher = typename IoJob::Launcher;
		using Callback = typename IoJob::Callback;

		template<class> class Ethernet;
		template<uint16_t, class, class = void> struct InterfaceManager;

		class Block;
		class Chain;
		class DataChain;
		class PacketChain;
		class PacketBuilder;

		class NullTag;
		class DstPortTag;
		class ConnectionTag;

		template<class> class PacketInputChannelBase;
		template<class> class PacketInputChannel;

		class ChunkReaderState;
		class PacketStreamState;
		template<class> class PacketReaderBase;
		template<class> class PacketWriterBase;
		template <class> class NullObserver;
		template <class> class ChecksumObserver;
		template<template <class> class> class PacketStreamBase;
		class SummedPacketStream;
		class PacketTransmissionRequest;

		typedef ::RoutingTable<Os, Interface, routingTableEntries> RoutingTable;


		class DummyDigester;

		class RxPacketHandler;

		template<class> class IpTxJob;
		template<class> class IpTransmitterBase;
		template<class, class> class IpRxJob;
		template<class, class = DummyDigester> class IpReplyJob;

		class PacketProcessor;
		template<class> class ArpCore;

		class IpCore;
		class RawCore;
		class IcmpCore;
		class UdpCore;
		class TcpCore;
		struct State;

		static State state;

		inline static constexpr uint16_t bytesToBlocks(size_t);

	public:
		struct Chunk;
		class Fixup;
		class PacketStream;
		class PacketAssembler;
		typedef BufferPool<Os, bufferCount, Block, txBufferLimit, rxBufferLimit> Pool;

		static constexpr auto blockMaxPayload = Block::dataSize;
		typedef typename Pool::Storage Buffers;
		using Route = typename RoutingTable::Route;

		class RawReceiver;
		class RawTransmitter;

		class UdpReceiver;
		class UdpTransmitter;

		class IcmpReceiver;

		class TcpListener;
		class TcpSocket;

		struct BufferStats;

		static inline void init(Buffers &buffers);

		static inline bool addRoute(const Route& route, bool setUp = false);

		template<template<class, uint16_t> class Driver>
		constexpr static inline auto* getEthernetInterface();

		static inline BufferStats getBufferStats();

		template<bool enabled = useDiagnosticCounters>
		static inline typename EnableIf<enabled, DiagnosticCounters>::Type getCounterStats();
	};
};

template<class S, class... Args>
using Network = NetworkOptions::Configurable<S, Args...>;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define NET_ASSERT(x)	Os::assert((x), "Internal error at " __FILE__ "@" STR(__LINE__))

#include "buffer/Block.h"
#include "buffer/Chain.h"
#include "buffer/Packet.h"
#include "buffer/DataChain.h"
#include "buffer/PacketChain.h"
#include "buffer/PacketStream.h"
#include "buffer/PacketBuilder.h"
#include "buffer/PacketAssembler.h"
#include "buffer/PacketReaderBase.h"
#include "buffer/PacketStreamBase.h"
#include "buffer/PacketWriterBase.h"
#include "buffer/PacketStreamState.h"
#include "buffer/StructuredAccessor.h"
#include "buffer/SummedPacketStream.h"
#include "buffer/PacketTransmissionRequest.h"

#include "ethernet/ArpCore.h"
#include "ethernet/Ethernet.h"
#include "ethernet/ArpReplyJob.h"
#include "ethernet/ArpResolver.h"

#include "icmp/IcmpCore.h"
#include "icmp/IcmpReceiver.h"
#include "icmp/IcmpDurPurReplyJob.h"
#include "icmp/IcmpEchoReplyJob.h"

#include "internal/Misc.h"
#include "internal/State.h"
#include "internal/Fixup.h"
#include "internal/Interface.h"
#include "internal/InterfaceManager.h"
#include "internal/PacketProcessor.h"
#include "internal/PacketInputChannel.h"

#include "ip/IpCore.h"
#include "ip/IpRxJob.h"
#include "ip/IpTxJob.h"
#include "ip/IpReplyJob.h"
#include "ip/IpTransmitterBase.h"

#include "raw/RawCore.h"
#include "raw/RawReceiver.h"
#include "raw/RawTransmitter.h"

#include "tcp/TcpRx.h"
#include "tcp/TcpTx.h"
#include "tcp/TcpCore.h"
#include "tcp/TcpAckJob.h"
#include "tcp/TcpRstJob.h"
#include "tcp/TcpSocket.h"
#include "tcp/TcpListener.h"
#include "tcp/TcpInputChannel.h"
#include "tcp/TcpRetransmitJob.h"

#include "udp/UdpCore.h"
#include "udp/UdpTransmitter.h"
#include "udp/UdpReceiver.h"

#endif /* NETWORK_H_ */
