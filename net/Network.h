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
#include "Packet.h"
#include "SharedTable.h"
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
		typedef typename Os::IoChannel Interface;
        struct Route;

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

			static bool done(typename Os::IoJob* item, typename Os::IoJob::Result result, const typename Os::IoJob::Reactivator &) {
			    auto self = static_cast<AllocateTxPacketJob*>(item);

				if(result == Os::IoJob::Result::Done) {
					 auto first = reinterpret_cast<typename TxPool::Block*>(item->param);
					 TxPacket* packet = new(first) TxPacket;

					 if(auto route = state.routingTable.findRoute(self->in.dst)) {
					     /* TODO fill in headers */
					     self->dev = route->dev;
	                     item->param = reinterpret_cast<uintptr_t>(static_cast<Packet*>(packet));
					 } else {
					     packet->~TxPacket();
					     item->param = 0;
					 }

					 // TODO table lookups and restructuring
				}

				return false;
			}

			inline void prepare(AddressIp4 dst, uint16_t port, size_t freeStorageSize) {
				in.dst = dst;
				in.port = port;
				const size_t nBlocks = (freeStorageSize /* TODO + header size */ + transmitBlockSize - 1) / transmitBlockSize;
				this->Os::IoJob::prepare(&AllocateTxPacketJob::done, nBlocks);
			}
		};

		struct SendTxPacketJob: Os::IoJob {
			friend class Os::IoChannel;

			static bool done(typename Os::IoJob* item, typename Os::IoJob::Result result, const typename Os::IoJob::Reactivator &) {
			    reinterpret_cast<TxPacket*>(item->param)->~TxPacket();
				return false;
			}

			inline void prepare() {
				this->Os::IoJob::prepare(&SendTxPacketJob::done);
			}
		};

	public:
		static inline void init() {
			state.txPool.init();
		}

		template<class C> constexpr static inline Interface *getIf() {
			return static_cast<TxBinder<C>*>(&state.interfaces);
		}

		struct PreparedPacket {
			Packet* packet;
			Interface* dev;
			void send() {
				typename Os::template IoRequest<SendTxPacketJob> req;
				req.init();
				dev->submit(&req);
				req.wait();
			}
		};

		static PreparedPacket prepareUdpPacket(AddressIp4 dst, uint16_t port, size_t size) {
			typename Os::template IoRequest<AllocateTxPacketJob> req;
			req.init();
			state.txPool.submit(&req, dst, port, size);
			req.wait();
			return {reinterpret_cast<Packet*>(req.param), req.dev};
		}

		static inline bool addRoute(const Route& route) {
		    return state.routingTable.add(route);
		}
	};
};

template<class S, class... Args>
typename NetworkOptions::Configurable<S, Args...>::State NetworkOptions::Configurable<S, Args...>::state;

template<class S, class... Args>
using Network = NetworkOptions::Configurable<S, Args...>;

template<class S, class... Args>
class Network<S, Args...>::Route {
    friend class Network<S, Args...>;
	AddressIp4 net;
	AddressIp4 src;
	Interface* dev;
	uint8_t mask;
	uint8_t metric;

public:
	inline Route(AddressIp4 net, uint8_t mask, AddressIp4 src, Interface* dev, uint8_t metric = 0):
	    net(net), src(src), dev(dev), mask(mask), metric(metric) {}
};

template<class S, class... Args>
class Network<S, Args...>::RoutingTable: SharedTable<Os, Route, routingTableEntries> {
public:
	bool add(const Route& route) {
	    auto entry = this->findOrAllocate([&route](Route* other){
	        return route.net == other->net && route.src == other->src && route.dev == other->dev && route.mask == other->mask;
	    });

	    if(entry->isValid()) {
	        entry->release();
	        return false;
	    }

	    *entry->getData() = route;
	    entry->finalize();
		return true;
	}

	Route* findRoute(const AddressIp4& dst)
	{
	    auto entry = this->findBest([&dst](Route* route){
            return (route->net / route->mask != dst / route->mask) ? 0 : (route->mask << 8) | route->metric;
        });

	    if(!entry)
	        return nullptr;

	    return entry->getData();
	}
};

template<class S, class... Args>
template<class... Input>
struct Network<S, Args...>::Ifs<typename NetworkOptions::Set<Input...>, void>: TxBinder<Input>...{};

template<class S, class... Args>
template<class If>
class Network<S, Args...>::TxBinder: Network<S, Args...>::Os::IoChannel {
	friend class Network<S, Args...>;
	virtual void enableProcess() override final {}
	virtual void disableProcess() override final {}
	virtual bool addJob(typename Os::IoJob*) override final {return true;}
	virtual bool removeJob(typename Os::IoJob*) override final {return true;}
};

template<class S, class... Args>
class Network<S, Args...>::TxPacket: public ChunkedPacket<TxPacket> {
    friend class ChunkedPacket<TxPacket>;
	size_t getSize() {
		return transmitBlockSize;
	}

	char* nextBlock() {
		auto currentBlock = reinterpret_cast<typename TxPool::Block*>(this->block());
		auto successorBlock = currentBlock->next;
		return reinterpret_cast<char*>(successorBlock);
	}

public:

	inline TxPacket(): ChunkedPacket<TxPacket>((char*)this, sizeof(*this)) {}

	inline virtual ~TxPacket() override final{
		auto first = reinterpret_cast<typename TxPool::Block*>(this);
		state.txPool.reclaim(first);
	}
};

#endif /* NETWORK_H_ */
