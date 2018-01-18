/*
 * Network.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "Routing.h"
#include "ArpTable.h"
#include "AddressIp4.h"
#include "BufferPool.h"
#include "SharedTable.h"
#include "AddressEthernet.h"
#include "NetErrorStrings.h"
#include "IpProtocolNumbers.h"
#include "DiagnosticCounters.h"
#include "InetChecksumDigester.h"

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
	PET_CONFIG_VALUE(MachineLittleEndian, bool);
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
		PET_EXTRACT_VALUE(swapBytes, MachineLittleEndian, true, Options);
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

		template<class> class Ethernet;
		template<uint16_t, class, class = void> struct Interfaces;

		class Block;
		class PacketChain;
		class PacketBuilder;

		class NullTag;
		class DstPortTag;
		class ConnectionTag;

		template<class> class PacketInputChannel;
		typedef PacketInputChannel<NullTag> IcmpInputChannel;
		typedef PacketInputChannel<NullTag> RawInputChannel;
		typedef PacketInputChannel<DstPortTag> UdpInputChannel;
		typedef PacketInputChannel<DstPortTag> TcpListenerChannel;
		class TcpInputChannel;

		template<class> class PacketWriterBase;
		class NullObserver;
		template<class> class ObservedPacketStream;
		class PacketTransmissionRequest;

	public:
		class Interface;
		struct Chunk;
		class Packet;
		class PacketStream;
		class PacketAssembler;
		class PacketDisassembler;
		typedef Scheduler Os;
		typedef BufferPool<Os, bufferCount, Block, txBufferLimit, rxBufferLimit> Pool;

	private:
		typedef ::RoutingTable<Os, Interface, routingTableEntries> RoutingTable;

		class DummyDigester;

		class RxPacketHandler;

		template<class> class IpTxJob;
		template<class> class IpTransmitterBase;
		template<class, class> class IpRxJob;
		template<class, class = DummyDigester> class IpReplyJob;
		class IcmpEchoReplyJob;
		class ArpReplyJob;
		class TcpAckJob;
		class TcpRstJob;

		class PacketProcessor;

		static struct State: DiagnosticCounterStorage<useDiagnosticCounters> {
			struct Ager: Os::Timeout {
				inline void age();
				static inline void doAgeing(typename Os::Sleeper*);
				inline Ager(): Os::Timeout(&Ager::doAgeing) {}
			} ager;

            inline void* operator new(size_t, void* x) { return x; }

            PacketProcessor rxProcessor;

            RawInputChannel rawInputChannel;

            IcmpEchoReplyJob icmpReplyJob;
			IcmpInputChannel icmpInputChannel;

			UdpInputChannel udpInputChannel;

			TcpAckJob tcpAckJob;
			TcpRstJob tcpRstJob;
			TcpListenerChannel tcpListenerChannel;
			TcpInputChannel tcpInputChannel;

			Interfaces<Block::dataSize, IfsToBeUsed> interfaces;
			RoutingTable routingTable;
			Pool pool;

			inline State();
		} state;

		static void fillInitialIpHeader(PacketBuilder &packet, AddressIp4 srcIp, AddressIp4 dstIp);

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

		static inline void tcpTxPostProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payloadLength);

		template<typename Pool::Quota> static inline Packet dropIpHeader(Packet packet);

		inline static constexpr uint16_t bytesToBlocks(size_t);

		using IoJob = typename Os::IoJob;
		using Result = typename IoJob::Result;
		using Launcher = typename IoJob::Launcher;
		using Callback = typename IoJob::Callback;

		template<class Reader> static inline RxPacketHandler* checkIcmpPacket(Reader&);
		template<class Reader> static inline RxPacketHandler* checkUdpPacket(Reader&, uint16_t);
		template<class Reader> static inline RxPacketHandler* checkTcpPacket(Reader&, uint16_t);
		static inline void processReceivedPacket(Packet start);
		static inline void processIcmpPacket(Packet start);
		static inline void processRawPacket(Packet start);
		static inline void processUdpPacket(Packet start);
		static inline void processTcpPacket(Packet start);
		static inline void ipPacketReceived(Packet packet, Interface* dev);

	public:
		static constexpr auto blockMaxPayload = Block::dataSize;
		typedef typename Pool::Storage Buffers;
		using Route = typename RoutingTable::Route;

		class IpTransmitter;
		class UdpTransmitter;

		class IpReceiver;
		class IcmpReceiver;
		class UdpReceiver;

		class TcpListener;
		class TcpSocket;

		struct BufferStats;

		static inline constexpr uint32_t correctEndian(uint32_t);
		static inline constexpr uint16_t correctEndian(uint16_t);

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

template<class S, class... Args>
typename Network<S, Args...>::State NetworkOptions::Configurable<S, Args...>::state;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define NET_ASSERT(x)	Os::assert((x), "Internal error at " __FILE__ "@" STR(__LINE__))

#include "Misc.h"
#include "Arp.h"
#include "Tcp.h"
#include "Udp.h"
#include "Icmp.h"
#include "Packet.h"
#include "Endian.h"
#include "Ethernet.h"
#include "Interfaces.h"
#include "IpReplyJob.h"
#include "IpReception.h"
#include "TcpListener.h"
#include "PacketStream.h"
#include "TcpReplyJobs.h"
#include "PacketBuilder.h"
#include "IpTransmission.h"
#include "PacketProcessor.h"
#include "PacketInputChannel.h"

#endif /* NETWORK_H_ */
