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

struct NetworkOptions {
	PET_CONFIG_VALUE(TransmitPoolSize, size_t);
	PET_CONFIG_VALUE(TransmitBlockSize, size_t);
	PET_CONFIG_VALUE(RoutingTableEntries, size_t);
	PET_CONFIG_TYPE(Interfaces);

	template<class... Members> struct Set {
		static constexpr size_t n = sizeof...(Members);
	};

	template<class Scheduler, class... Options>
	class Configurable {
		typedef Scheduler Os;
		PET_EXTRACT_VALUE(transmitPoolSize, TransmitPoolSize, 64, Options);
		PET_EXTRACT_VALUE(transmitBlockSize, TransmitBlockSize, 64, Options);
		PET_EXTRACT_VALUE(routingTableEntries, RoutingTableEntries, 4, Options);
		PET_EXTRACT_TYPE(IfsToBeUsed, Interfaces, Set<>, Options);

		static_assert(sizeof(Packet) <= transmitBlockSize, "Buffer block size smaller than internal packet header size");
		static_assert(IfsToBeUsed::n, "No interfaces specified");

	public:
		class Interface;
		class Route;

	private:
		template<class> class TxBinder;
		template<class, class = void> struct Ifs;

		class TxPacket;
		typedef BufferPool<Os, transmitPoolSize, transmitBlockSize, TxPacket> TxPool;

		struct RoutingTable;

		static struct State {
			TxPool txPool;
			Ifs<IfsToBeUsed> interfaces;
			RoutingTable routingTable;
		} state;

		struct AllocateTxPacketJob: Os::IoJob {
			friend class Os::IoChannel;
			union {
				struct {
					AddressIp4 dst;
					uint16_t port;
				} in;
				Interface* dev;
			};

			static bool done(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
			    auto self = static_cast<AllocateTxPacketJob*>(item);

				if(result == Os::IoJob::Result::Done) {
					 auto first = reinterpret_cast<typename TxPool::Block*>(item->param);
					 TxPacket* packet = new(first) TxPacket;

					 if(auto route = state.routingTable.findRoute(self->in.dst)) {
					     /* TODO fill in headers */
					     self->dev = route->dev;
	                     item->param = reinterpret_cast<uintptr_t>(packet);
					 } else {
					     packet->~TxPacket();
					     item->param = 0;
					 }

					 // TODO table lookups and restructuring
				}

				return false;
			}

			inline bool start(AddressIp4 dst, uint16_t port, size_t freeStorageSize, typename Os::IoJob::Hook hook = 0) {
				in.dst = dst;
				in.port = port;
				const size_t nBlocks = (freeStorageSize /* TODO + header size */ + transmitBlockSize - 1) / transmitBlockSize;
				return this->submit(hook, &state.txPool, &AllocateTxPacketJob::done, nBlocks);
			}
		};

	public:
		static inline void init() {
			state.txPool.init();
			state.interfaces.init();
		}

		struct PreparedPacket {
			TxPacket* packet;
			Interface* dev;
			void send() {
				struct SendPacket: Os::IoJob {
					static bool nop(typename Os::IoJob* item, typename Os::IoJob::Result result, typename Os::IoJob::Hook hook) {
						return false;
					}

				public:
					bool start(Interface *dev, TxPacket* packet, typename Os::IoJob::Hook hook = 0) {
						return this->submit(hook, dev, &SendPacket::nop, reinterpret_cast<uintptr_t>(packet));
					}
				};

				typename Os::template IoRequest<SendPacket> req;
				req.init();
				req.start(dev, packet);
				req.wait();
			}
		};

		static PreparedPacket prepareUdpPacket(AddressIp4 dst, uint16_t port, size_t size) {
			typename Os::template IoRequest<AllocateTxPacketJob> req;
			req.init();
			req.start(dst, port, size);
			req.wait();
			return {reinterpret_cast<TxPacket*>(req.param), req.dev};
		}

		static inline bool addRoute(const Route& route) {
		    return state.routingTable.add(route);
		}

		template<class C> constexpr static inline Interface *getIf();
		template<class C> constexpr static inline TxPacket *getEgressPacket();
	};
};

template<class S, class... Args>
typename NetworkOptions::Configurable<S, Args...>::State NetworkOptions::Configurable<S, Args...>::state;

template<class S, class... Args>
using Network = NetworkOptions::Configurable<S, Args...>;

#include "Routing.h"
#include "Interfaces.h"
#include "TxPacket.h"

#endif /* NETWORK_H_ */
