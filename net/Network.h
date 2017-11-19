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
		class Interface;

	private:
		template<class> class TxBinder;
		template<class> class RxBinder;
		template<class> class InterfaceBinder;
		template<class, class = void> struct Ifs;

		class TxPacket;
		typedef BufferPool<Os, transmitPoolSize, transmitBlockSize, TxPacket> TxPool;

		struct Route;
		struct RoutingTable;

		static struct State {
			TxPool txPool;
			Ifs<IfsToBeUsed> interfaces;
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
				if(result == Os::IoJob::Result::Done) {
					 auto first = reinterpret_cast<typename TxPool::Block*>(item->param);
					 new(first) TxPacket;

					 // TODO table lookups and restructuring
				}

				return false;
			}

			inline void prepare(AddressIp4 dst, uint16_t port, size_t freeStorageSize) {
				in.dst = dst;
				in.port = port;
				const size_t nBlocks = (freeStorageSize + transmitBlockSize - 1) / transmitBlockSize;
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
			return static_cast<InterfaceBinder<C>*>(&state.interfaces);
		}

		struct PreparedPacket {
			Packet* packet;
			Interface* dev;
			void send() {
				typename Os::template IoRequest<SendTxPacketJob> req;
				req.init();
				dev->getTx()->submit(&req);
				req.wait();
			}
		};

		static PreparedPacket prepareUdpPacket(AddressIp4 dst, uint16_t port, size_t size) {
			typename Os::template IoRequest<AllocateTxPacketJob> req;
			req.init();
			state.txPool.submit(&req, dst, port, size);
			req.wait();
			return {reinterpret_cast<TxPacket*>(req.param), req.dev};
		}
	};
};

template<class S, class... Args>
typename NetworkOptions::Configurable<S, Args...>::State NetworkOptions::Configurable<S, Args...>::state;

template<class S, class... Args>
using Network = NetworkOptions::Configurable<S, Args...>;

template<class S, class... Args>
struct Network<S, Args...>::Route {
	AddressIp4 net;
	AddressIp4 src;
	// ??? *dev;
	uint8_t mask;
	uint8_t metric;
public:

};

template<class S, class... Args>
class Network<S, Args...>::RoutingTable: SharedTable<Os, Route, routingTableEntries> {
public:
	bool add(const Route& route) {
		return true;
	}

	Route* findRoute(const AddressIp4& dst) {
		return nullptr;
	}
};

template<class S, class... Args>
class Network<S, Args...>::Interface {
	friend class Network<S, Args...>;
	virtual typename Os::IoChannel *getTx() = 0;
	virtual typename Os::IoChannel *getRx() = 0;
};

template<class S, class... Args>
template<class... Input>
class Network<S, Args...>::Ifs<typename NetworkOptions::Set<Input...>, void>: InterfaceBinder<Input>...{};

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
template<class If>
class Network<S, Args...>::RxBinder: Network<S, Args...>::Os::IoChannel {
	friend class Network<S, Args...>;
	virtual void enableProcess() override final {}
	virtual void disableProcess() override final {}
	virtual bool addJob(typename Os::IoJob*) override final {return true;}
	virtual bool removeJob(typename Os::IoJob*) override final {return true;}
};

template<class S, class... Args>
template<class If>
class Network<S, Args...>::InterfaceBinder:
	Network<S, Args...>::Interface,
	Network<S, Args...>::template TxBinder<If>,
	Network<S, Args...>::template RxBinder<If>
{
	virtual typename Os::IoChannel *getTx() override final {
		return static_cast<TxBinder<If>*>(this);
	}

	virtual typename Os::IoChannel *getRx() override final {
		return static_cast<RxBinder<If>*>(this);
	}
};

template<class S, class... Args>
class Network<S, Args...>::TxPacket: public ChunkedPacket {
	virtual size_t getSize() override final {
		return transmitBlockSize;
	}

	virtual char* nextBlock() override final {
		auto currentBlock = reinterpret_cast<typename TxPool::Block*>(block());
		auto successorBlock = currentBlock->next;
		return reinterpret_cast<char*>(successorBlock);
	}

public:

	inline TxPacket(): ChunkedPacket((char*)this, sizeof(*this)) {}

	inline virtual ~TxPacket() {
		auto first = reinterpret_cast<typename TxPool::Block*>(this)->next;
		state.txPool.reclaim(first);
	}
};


#endif /* NETWORK_H_ */
