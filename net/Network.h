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

		struct RoutingTable;

		static struct State {
			Ifs<IfsToBeUsed> interfaces;
			RoutingTable routingTable;
		} state;

		struct UdpTxJob: Os::IoJob {
			friend class Os::IoChannel;
			AddressIp4 dst;
			uint16_t port;
			Interface* dev;

			union {
				BufferPoolIoData<Os> poolParams;
				TxPacket packet;
			};

			static bool done(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
			    auto self = static_cast<UdpTxJob*>(item);

				if(result == Os::IoJob::Result::Done) {
					void* firstBlock = self->poolParams.first;
					//self->packet.init(firstBlock, 0, self->dev->getStandardPacketOperations());
				}

				return false;
			}

			inline bool start(AddressIp4 dst, uint16_t port, size_t freeStorageSize, typename Os::IoJob::Hook hook = 0)
			{
				if(auto route = state.routingTable.findRoute(dst)) { // TODO fill in headers
					this->dev = route->dev;
					this->dst = dst;
					this->port = port;

					 // TODO ARP lookup

					poolParams.size = freeStorageSize /* TODO + header size */;
					return this->submit(hook, this->dev->getAllocator(), &UdpTxJob::done, &poolParams);
				}

				return false;
			}

			inline bool start(typename Os::IoJob::Hook hook = 0) {
				// TODO submit to sender
				return false;
			}
		};


	public:
		static inline void init() {
			state.interfaces.init();
		}

		struct UdpTransmission: private Os::template IoRequest<UdpTxJob> {
			bool prepare(AddressIp4 dst, uint16_t port, size_t size) {
				// TODO check state somehow
				// TODO block if needed
				return this->start(dst, port, size);
			}

			bool send() {
				// TODO check state somehow
				// TODO block if needed
				return this->start();
			}

			bool fill(const char* data, size_t length) {
				// TODO check state somehow
				//return this->packet.copyIn(data, length);
				return false;
			}
		};

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
