/*
 * NetPacket.cpp
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

static const char c55[] = "0123456789012345678901234567890123456789012345678901234";
static const char c57[] = "012345678901234567890123456789012345678901234567890123456";

using Net = Net64;
using Accessor = NetworkTestAccessor<Net>;

TEST_GROUP(NetPacket) {
	template<class Child>
	struct TaskBase: TestTask<Child> {
		Accessor::PacketBuilder builder;

		void init(size_t n) {
			builder.init(Accessor::pool.allocateDirect<Accessor::Pool::Quota::Tx>(n));
		}

		template<class Reader>
		bool readAndCheck(Reader &reader, const char* str) {
			while(*str) {
				if(reader.atEop()) return TaskBase::TestTask::bad;
				uint8_t ret;
				if(!reader.read8(ret)) return TaskBase::TestTask::bad;
				if(ret != *str++) return TaskBase::TestTask::bad;
			}

			return TaskBase::TestTask::ok;
		}

		bool checkStreamContent(const char* str) {
			Accessor::PacketStream reader(builder);
			return readAndCheck(reader, str);
		}

		bool checkDirectStreamContent(const char* str) {
			Accessor::PacketStream reader(builder);

			while(*str) {
				Net::Chunk chunk = reader.getChunk();

				if(!chunk.length) return TaskBase::TestTask::bad;
				if(*chunk.start != *str++) return TaskBase::TestTask::bad;

				reader.advance(1);
			}

			return TaskBase::TestTask::ok;
		}

		bool checkBlockContent(const char* str) {
			Accessor::PacketStream reader(builder);

			size_t length = strlen(str);

			do {
				Net::Chunk chunk = reader.getChunk();
				size_t runLength = chunk.length < length ? chunk.length : length;

				if(memcmp(chunk.start, str, runLength) != 0)
					return TaskBase::TestTask::bad;

				str += runLength;
				length -= runLength;
				reader.advance(static_cast<uint16_t>(runLength));
			}while(!reader.atEop());

			if(length) return TaskBase::TestTask::bad;

			return TaskBase::TestTask::ok;
		}
	};

	typedef Accessor::PacketBuilder::PacketWriterBase Writer;
	typedef Accessor::PacketStream::PacketStreamBase Reader;

	template<const char* padding, class Data, bool (Writer::* write)(Data), bool(Reader::* read)(Data&)>
	struct OverlapTask: TaskBase<OverlapTask<padding, Data, write, read>> {
		static constexpr Data value1 = static_cast<Data>(0xb16b00b5);
		static constexpr Data value2 = static_cast<Data>(0x600dc0de);

		bool run() {
			this->init(2);
			if(this->builder.copyIn(padding, strlen(padding)) != strlen(padding)) return OverlapTask::bad;
			if(!(this->builder.*write)(value1)) return OverlapTask::bad;
			if(!(this->builder.*write)(value2)) return OverlapTask::bad;
			this->builder.done();

			Accessor::PacketStream reader(this->builder);

			if(this->readAndCheck(reader, padding) == OverlapTask::bad)
				return OverlapTask::bad;

			Data ret;

			if(!(reader.*read)(ret)) return OverlapTask::bad;
			if(ret != value1) return OverlapTask::bad;

			if(!(reader.*read)(ret)) return OverlapTask::bad;
			if(ret != value2) return OverlapTask::bad;

            if((reader.*read)(ret)) return OverlapTask::bad;

			return OverlapTask::ok;
		}
	};

	template<class Task>
	void work(Task& task) {
		task.start();
		Net::init(NetBuffers<Net>::buffers);
		CommonTestUtils::start();
		CHECK(!task.error);
	}

	static constexpr const char* hello = "hello";
	static constexpr const char* world = "world";
	static constexpr const char* helloWorld = "hello world\n";
	static constexpr const char* lipsum =  "Lorem ipsum dolor sit amet, "
			"consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore.";
	static constexpr const char* foobar = "foobar";
};

TEST(NetPacket, BuildSimple) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(1);
			if(builder.copyIn(hello, 5) != 5) return bad;
			if(!builder.write8(' ')) return bad;
			if(builder.copyIn(world, 5) != 5) return bad;
			if(!builder.write8('\n')) return bad;
			builder.done();

			return checkStreamContent(helloWorld);
		}
	} task;

	work(task);
}

TEST(NetPacket, BuildMulti) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(lipsum, strlen(lipsum)) != strlen(lipsum)) return bad;
			builder.done();

			return checkStreamContent(lipsum);
		}
	} task;

	work(task);
}

TEST(NetPacket, BuildMultiDirectCheck) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(lipsum, strlen(lipsum)) != strlen(lipsum)) return bad;
			builder.done();

			return checkDirectStreamContent(lipsum);
		}
	} task;

	work(task);
}

TEST(NetPacket, BuildMultiBlockCheck) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(lipsum, strlen(lipsum)) != strlen(lipsum)) return bad;
			builder.done();

			return checkBlockContent(lipsum);
		}
	} task;

	work(task);
}

TEST(NetPacket, FreeSpare) {

	struct Task: TaskBase<Task> {
		bool run() {
			init(4);
			if(Accessor::pool.statTxUsed() != 4) return bad;
			if(builder.copyIn(foobar, strlen(foobar)) != strlen(foobar)) return bad;

			builder.done();

			if(Accessor::pool.statTxUsed() != 1) return bad;

			return checkStreamContent(foobar);
		}
	} task;

	work(task);
}

TEST(NetPacket, Overread) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(hello, strlen(hello)) != strlen(hello)) return bad;
			builder.done();

			Accessor::PacketStream reader(builder);

			char temp[strlen(hello)+2];
			if(reader.copyOut(temp, sizeof(temp)) != strlen(hello)) return bad;
			if(strcmp(temp, hello) != 0) return bad;

			return ok;
		}
	} task;

	work(task);
}

TEST(NetPacket, OverreadPartial) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(hello, strlen(hello)) != strlen(hello)) return bad;

			Accessor::PacketStream reader(builder);

			char temp[strlen(hello)+2];
			if(reader.copyOut(temp, sizeof(temp)) != 0) return bad;

			return ok;
		}
	} task;

	work(task);
}

TEST(NetPacket, OverreadDirect) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(hello, strlen(hello)) != strlen(hello)) return bad;
			builder.done();

			if(checkDirectStreamContent(helloWorld) != bad)
				return bad;

			return ok;
		}
	} task;

	work(task);
}

TEST(NetPacket, WriteExhaust) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(1);
			if(builder.copyIn(lipsum, strlen(lipsum)) != 60) return bad;
			builder.done();

			if(checkStreamContent(lipsum) != bad)
				return bad;

			return ok;
		}
	} task;

	work(task);
}

TEST(NetPacket, WordOverlapNet) {
	OverlapTask<
			c55,
			uint32_t,
			&Accessor::PacketBuilder::write32net,
			&Accessor::PacketStream::read32net
	> task;
	work(task);
}

TEST(NetPacket, HalfWordOverlapNet) {
	OverlapTask<
			c57,
			uint16_t,
			&Accessor::PacketBuilder::write16net,
			&Accessor::PacketStream::read16net
	> task;
	work(task);
}

TEST(NetPacket, WordOverlapRawt) {
	OverlapTask<
			c55,
			uint32_t,
			&Accessor::PacketBuilder::write32raw,
			&Accessor::PacketStream::read32raw
	> task;
	work(task);
}

TEST(NetPacket, HalfWordOverlapRaw) {
	OverlapTask<
			c57,
			uint16_t,
			&Accessor::PacketBuilder::write16raw,
			&Accessor::PacketStream::read16raw
	> task;
	work(task);
}

TEST(NetPacket, SimplePatch) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(hello, strlen(hello)) != strlen(hello)) return bad;
			builder.done();

			Accessor::PacketStream modifier(builder);

			if(modifier.copyIn(world, strlen(world)) != strlen(world))
					return bad;

			if(checkDirectStreamContent(world) != ok)
				return bad;

			return ok;
		}
	} task;

	work(task);
}

TEST(NetPacket, ComplexPatch) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);

			builder.write8(123);

			for(int i=0; i<27; i++)
				if(!builder.write32net(i)) return bad;

			builder.done();

			Accessor::PacketStream modifier(builder);

			uint8_t ret;
			if(!modifier.read8(ret)) return bad;
			if(ret != 123) return bad;

			for(int i=0; i<9; i++) {
			    uint32_t ret;
				if(!modifier.read32net(ret)) return bad;
				if(ret != (3u * i)) return bad;
				if(!modifier.write32net(3 * i)) return bad;
				if(!modifier.skipAhead(4)) return bad;
			}

			if(modifier.skipAhead(4)) return bad;

			Accessor::PacketStream reader(builder);

            if(!reader.read8(ret)) return bad;
            if(ret != 123) return bad;

			for(int i=0; i<9; i++) {
                uint32_t ret;
                if(!reader.read32net(ret)) return bad;
				if(ret != (3u * i)) return bad;

				if(!reader.read32net(ret)) return bad;
				if(ret != (3u * i)) return bad;

				if(!reader.read32net(ret)) return bad;
				if(ret != (3u * i + 2u)) return bad;
			}

			return ok;
		}
	} task;

	work(task);
}

TEST(NetPacket, Indirect) {
    struct Task: TaskBase<Task> {
        bool run() {
            init(5);

            if(!builder.write8('|')) return bad;
            if(!builder.addByReference(hello, strlen(hello))) return bad;
            if(!builder.write8('|')) return bad;
            if(!builder.addByReference(world, strlen(world))) return bad;
            if(!builder.write8('|')) return bad;
            builder.done();

            return checkStreamContent("|hello|world|");
        }
    } task;

    work(task);
}

TEST(NetPacket, OffsetDropSimple) {
    struct Task: TaskBase<Task> {
        bool checkFrom(Net::Packet p, uint8_t from)
        {
            Net::PacketStream reader(p);

            for(uint8_t i = from; i < 3 * Net::blockMaxPayload; i++) {
                uint8_t ret;
                if(!reader.read8(ret)) return false;
                if(ret != i) return false;
            }

            return reader.atEop();
        }

        bool run() {
            init(3);

            for(uint8_t i=0; i< 3 * Net::blockMaxPayload; i++)
                if(!builder.write8(i)) return bad;

            builder.done();
            if(Accessor::pool.statTxUsed() != 3) return Task::bad;
            Net::Packet p = builder;

            if(!checkFrom(p, 0)) return Task::bad;

            for(uint8_t i=1; i<Net::blockMaxPayload; i++) {
            	if(!p.dropInitialBytes<Accessor::Pool::Quota::Tx>(1)) return Task::bad;
                if(Accessor::pool.statTxUsed() != 3) return Task::bad;
                if(!checkFrom(p, i)) return Task::bad;
            }

            if(!p.dropInitialBytes<Accessor::Pool::Quota::Tx>(1)) return Task::bad;
            if(Accessor::pool.statTxUsed() != 2) return Task::bad;
            if(!checkFrom(p, Net::blockMaxPayload)) return Task::bad;

            if(!p.dropInitialBytes<Accessor::Pool::Quota::Tx>(Net::blockMaxPayload + 1)) return Task::bad;
            if(Accessor::pool.statTxUsed() != 1) return Task::bad;

            if(!checkFrom(p, 2 * Net::blockMaxPayload + 1)) return Task::bad;

            if(p.dropInitialBytes<Accessor::Pool::Quota::Tx>(Net::blockMaxPayload * 100)) return Task::bad;
            if(Accessor::pool.statTxUsed() != 0) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}

TEST(NetPacket, OffsetDropAtOnce) {
    struct Task: TaskBase<Task> {
        bool run() {
            init(3);

            for(uint8_t i=0; i< 3 * Net::blockMaxPayload; i++)
                if(!builder.write8(i)) return bad;

            builder.done();
            if(Accessor::pool.statTxUsed() != 3) return Task::bad;
            Net::Packet p = builder;

            if(p.dropInitialBytes<Accessor::Pool::Quota::Tx>(Net::blockMaxPayload * 100)) return Task::bad;
            if(Accessor::pool.statTxUsed() != 0) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}

TEST(NetPacket, OffsetIndirect) {
    struct Task: TaskBase<Task> {
    	bool helloDestroyed = false;
    	bool worldDestroyed = false;

    	static inline void destroy(void* user, const char* data, uint32_t length) {
    		if(data == hello)
    			reinterpret_cast<Task*>(user)->helloDestroyed = true;
    		else if(data == world)
    			reinterpret_cast<Task*>(user)->worldDestroyed = true;
    	}

        bool run() {
            init(5);

            if(!builder.write8('|')) return bad;
            if(!builder.addByReference(hello, strlen(hello), destroy, this)) return bad;
            if(!builder.write8('|')) return bad;
            if(!builder.addByReference(world, strlen(world), destroy, this)) return bad;
            if(!builder.write8('|')) return bad;
            builder.done();

            if(!builder.dropInitialBytes<Accessor::Pool::Quota::Tx>(3)) return Task::bad;

            if(helloDestroyed || worldDestroyed) return bad;
            if(checkStreamContent("llo|world|") != ok) return bad;

            if(!builder.dropInitialBytes<Accessor::Pool::Quota::Tx>(3)) return Task::bad;
            if(!helloDestroyed || worldDestroyed) return bad;
            if(checkStreamContent("|world|") != ok) return bad;

            if(!builder.dropInitialBytes<Accessor::Pool::Quota::Tx>(2)) return Task::bad;
            if(!helloDestroyed || worldDestroyed) return bad;
            if(checkStreamContent("orld|") != ok) return bad;

            if(!builder.dropInitialBytes<Accessor::Pool::Quota::Tx>(4)) return Task::bad;
            if(!helloDestroyed || !worldDestroyed) return bad;

            if(builder.dropInitialBytes<Accessor::Pool::Quota::Tx>(1)) return Task::bad;

            if(Accessor::pool.statTxUsed()) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}


TEST(NetPacket, Chain) {
    struct Task: TaskBase<Task> {
    	bool helloDestroyed = false;
    	bool worldDestroyed = false;

    	static inline void destroy(void* user, const char* data, uint32_t length) {
    		if(data == hello)
    			reinterpret_cast<Task*>(user)->helloDestroyed = true;
    		else if(data == world)
    			reinterpret_cast<Task*>(user)->worldDestroyed = true;
    	}

        bool run() {
            init(2);

            if(!builder.write8('|')) return bad;
            if(!builder.addByReference(hello, strlen(hello), destroy, this)) return bad;

            builder.done();
            Net::Packet p1 = builder;
            init(3);

            if(!builder.write8('|')) return bad;
            if(!builder.addByReference(world, strlen(world), destroy, this)) return bad;
            if(!builder.write8('|')) return bad;
            builder.done();
            Net::Packet p2 = builder;

            Accessor::PacketChain chain;

            if(!chain.isEmpty()) return bad;
            chain.put(p1);
            if(chain.isEmpty()) return bad;

            chain.reset();

            if(!chain.isEmpty()) return bad;
            chain.put(p1);
            if(chain.isEmpty()) return bad;

            chain.put(p2);

            Net::Packet readback;
            Net::PacketStream reader;

            if(!chain.take(readback)) return bad;
            reader.init(readback);
            if(readAndCheck(reader, "|hello") != ok) return bad;
            readback.dispose<Accessor::Pool::Quota::Tx>();

            if(!chain.take(readback)) return bad;
            reader.init(readback);
            if(readAndCheck(reader, "|world|") != ok) return bad;
            readback.dispose<Accessor::Pool::Quota::Tx>();

            if(chain.take(readback)) return bad;

            if(Accessor::pool.statTxUsed()) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetPacket, ChainFlip) {
    struct Task: TaskBase<Task> {
    	bool helloDestroyed = false;
    	bool worldDestroyed = false;

    	static inline void destroy(void* user, const char* data, uint32_t length) {
    		if(data == hello)
    			reinterpret_cast<Task*>(user)->helloDestroyed = true;
    		else if(data == world)
    			reinterpret_cast<Task*>(user)->worldDestroyed = true;
    	}

        bool run() {
            init(2);

            if(!builder.write8('|')) return bad;
            if(!builder.addByReference(hello, strlen(hello), destroy, this)) return bad;

            builder.done();
            Net::Packet p1 = builder;
            init(3);

            if(!builder.write8('|')) return bad;
            if(!builder.addByReference(world, strlen(world), destroy, this)) return bad;
            if(!builder.write8('|')) return bad;
            builder.done();
            Net::Packet p2 = builder;

            Accessor::Packet::Accessor::getFirst(p2)->findEndOfPacket()->setNext(
            		Accessor::Packet::Accessor::getFirst(p1)
			);

            Accessor::PacketChain chain = Accessor::PacketChain::flip(Accessor::Packet::Accessor::getFirst(p2));

            Net::Packet readback;
            Net::PacketStream reader;

            if(!chain.take(readback)) return bad;
            reader.init(readback);
            if(readAndCheck(reader, "|hello") != ok) return bad;
            readback.dispose<Accessor::Pool::Quota::Tx>();

            if(!chain.take(readback)) return bad;
            reader.init(readback);
            if(readAndCheck(reader, "|world|") != ok) return bad;
            readback.dispose<Accessor::Pool::Quota::Tx>();

            if(chain.take(readback)) return bad;

            if(Accessor::pool.statTxUsed()) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetPacket, DataChain) {
    struct Task: TaskBase<Task> {
    	bool helloDestroyed = false;
    	bool worldDestroyed = false;

    	static inline void destroy(void* user, const char* data, uint32_t length) {
    		if(data == hello)
    			reinterpret_cast<Task*>(user)->helloDestroyed = true;
    		else if(data == world)
    			reinterpret_cast<Task*>(user)->worldDestroyed = true;
    	}

        bool run() {
            Accessor::DataChain chain;
            Accessor::PacketStream reader;

            init(1);
			if(builder.copyIn(hello, 5) != 5) return bad;
			if(!builder.write8(' ')) return bad;
			if(builder.copyIn(world, 5) != 5) return bad;
			if(!builder.write8('\n')) return bad;
			builder.done();											// 'hello world\n'
			builder.dropInitialBytes<Accessor::Pool::Quota::Tx>(4); // 'o world\n'

			chain.put(builder); 									// 'o world\n'

			init(3);
            if(!builder.addByReference(hello, strlen(hello), destroy, this)) return bad;
            if(builder.copyIn(" cruel ", 7) != 7) return bad;
            if(!builder.addByReference(world, strlen(world), destroy, this)) return bad;
            														//  hello     world
            builder.done();											// '^| cruel |^'

            builder.dropInitialBytes<Accessor::Pool::Quota::Tx>(4); //  hello         world
            														//     '^| cruel |^'

            														//       hello         world
            chain.put(builder);										// 'o world\n^| cruel |^'

            Accessor::DataChain::Packetizatation p;

            if(chain.packetize(p, 22)) return bad;

            if(!chain.packetize(p, 21)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "o world\no cruel world") != ok) return bad;

            chain.revert(p);

            if(!chain.packetize(p, 2)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "o ") != ok) return bad;

            chain.apply<Accessor::Pool::Quota::Tx>(p);
            if(Accessor::pool.statTxUsed() != 4) return Task::bad;
            if(helloDestroyed || worldDestroyed) return bad;

            if(!chain.packetize(p, 19)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "world\no cruel world") != ok) return bad;

            chain.revert(p);

            if(!chain.packetize(p, 10)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "world\no cr") != ok) return bad;

            chain.apply<Accessor::Pool::Quota::Tx>(p);
            if(Accessor::pool.statTxUsed() != 2) return Task::bad;
            if(!helloDestroyed || worldDestroyed) return bad;

            if(!chain.packetize(p, 4)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "uel ") != ok) return bad;

            chain.apply<Accessor::Pool::Quota::Tx>(p);
            if(Accessor::pool.statTxUsed() != 1) return Task::bad;

            if(!chain.packetize(p, 5)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "world") != ok) return bad;

            chain.revert(p);

            if(!chain.packetize(p, 2)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "wo") != ok) return bad;

            chain.apply<Accessor::Pool::Quota::Tx>(p);

            if(!chain.packetize(p, 2)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "rl") != ok) return bad;

            chain.apply<Accessor::Pool::Quota::Tx>(p);

            if(chain.packetize(p, 2)) return bad;

            if(!chain.packetize(p, 1)) return bad;
            reader.init(p);
            if(readAndCheck(reader, "d") != ok) return bad;

            chain.apply<Accessor::Pool::Quota::Tx>(p);

            if(chain.packetize(p, 1)) return bad;
            if(chain.packetize(p, 0)) return bad;

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(!helloDestroyed || !worldDestroyed) return bad;

            return ok;
        }
    } task;

    work(task);
}

TEST(NetPacket, ValidatorSimple) {
	struct Task: TaskBase<Task> {
	        bool run() {
	        							/* some junk      |      first block      |  second block frag */
	        	const uint8_t header[] = {0xba, 0xda, 0x55, 0x41, 0x10, 0xbe, 0xef, 'h', 'e', 'l', 'l'};
	        							/* second block end fragment  |  some unchecked data */
	        	const uint8_t data[] = {'o', 'w', 'o', 'r', 0x4c, 0x44, 0x50, 0x3e, 0xda, 0x7a};
	            init(2);

	            if(!builder.copyIn((const char*)header, sizeof(header))) return bad;
	            if(!builder.addByReference((const char*)data, sizeof(data))) return bad;
	            builder.done();

	            Accessor::SummedPacketStream stream(builder);

	            if(!stream.skipAhead(3)) return bad;	// skip initial junk

	            uint16_t ret;
	            if(!stream.read16net(ret)) return bad;	// header length kind of read
	            if(ret != 0x4110) return bad;

	            stream.start(2);						// check the rest of the header

	            if(!stream.finish()) return bad;		// finish block and check against already read part
	            if(stream.result() == static_cast<uint16_t>(~0x4110u)) return bad;

	            stream.start(10); // 'hellowor' + checksum
	            if(!stream.finish()) return bad;
	            if(stream.result()) return bad;

	            if(!stream.read16net(ret)) return bad;
	            if(ret != 0x503e) return bad;

	            if(!stream.goToEnd()) return bad;
	            if(stream.getLength() != (sizeof(data) + sizeof(header))) return bad;

	            return ok;
	        }
    } task;

    work(task);
}

TEST(NetPacket, GeneratorSimple) {
	struct Task: TaskBase<Task> {
	        bool run() {
	        							/* l2 sort of junk      | header 1st part */
	        	const uint8_t header[] = {0x30, 0xda, 0xfa, 0xca, 0x42, 0, 0, 0};
	        							/* header 2nd | actual payload part one */
	        	const uint8_t header2[] = {0, 8, 0, 8, 'h', 'e', 'l', 'l', 'o'};

	            init(3);
	            if(!builder.copyIn((const char*)header, sizeof(header))) return bad;
	            if(!builder.addByReference((const char*)header2, sizeof(header2))) return bad;
	            if(!builder.addByReference(world, strlen(world))) return bad;
	            builder.done();

	            Accessor::SummedPacketStream stream(builder);

	            if(!stream.skipAhead(4)) return bad;	// skip initial junk

	            uint16_t ret;
	            if(!stream.read16net(ret)) return bad;	// read some header info
	            if(ret != 0x4200) return bad;
	            stream.start(6);						// set to check rest of header

	            stream.patchNet(static_cast<uint16_t>(0x4200));
	            	//update digester with already read data.

	            stream.finish();						// go through the rest of the header

	            if(stream.getReducedState() != Accessor::correctEndian(static_cast<uint16_t>(~0xbdef))) return bad;

	            stream.start(8); // 'hellowor'
	            stream.finish();
	            if(stream.getReducedState() != Accessor::correctEndian(static_cast<uint16_t>(~0x4c44))) return bad;

	            uint8_t ret8;
	            if(!stream.read8(ret8)) return bad;
	            if(ret8 != 'l') return bad;

	            if(!stream.goToEnd()) return bad;
	            if(!stream.goToEnd()) return bad;
	            if(stream.getLength() != (strlen(world) + sizeof(header) + sizeof(header2))) return bad;

	            return ok;
	        }
    } task;

    work(task);
}

TEST(NetPacket, CopyFrom) {
    struct Task: TaskBase<Task> {
            bool run() {
                init(4);

                if(!builder.addByReference(helloWorld, strlen(helloWorld))) return bad;
                if(!builder.write8('\n')) return bad;
                if(!builder.copyIn(lipsum, strlen(lipsum))) return bad;
                if(!builder.write8('\n')) return bad;
                if(!builder.addByReference(foobar, strlen(foobar))) return bad;

                builder.done();
                Accessor::PacketStream reader(builder);

                Accessor::PacketBuilder copy;
                copy.init(Accessor::pool.allocateDirect<Accessor::Pool::Quota::Tx>(4));

                static constexpr auto totalLength = strlen(helloWorld) + strlen(lipsum) + strlen(foobar) + 2;
                if(copy.copyFrom(reader, totalLength) != totalLength) return bad;
                copy.done();

                Accessor::PacketStream checker(copy);

                if(readAndCheck(checker, helloWorld) != ok) return bad;
                if(readAndCheck(checker, "\n") != ok) return bad;
                if(readAndCheck(checker, lipsum) != ok) return bad;
                if(readAndCheck(checker, "\n") != ok) return bad;
                if(readAndCheck(checker, foobar) != ok) return bad;

                return ok;
            }
    } task;

    work(task);
}

TEST(NetPacket, CopySizeError) {
    struct Task: TaskBase<Task> {
            bool run() {
                init(3);

                if(!builder.copyIn(lipsum, strlen(lipsum))) return bad;
                builder.done();

                {
                    Accessor::PacketStream reader(builder);
                    Accessor::PacketBuilder copy;
                    copy.init(Accessor::pool.allocateDirect<Accessor::Pool::Quota::Tx>(Accessor::bytesToBlocks(strlen(lipsum))));
                    if(copy.copyFrom(reader, strlen(lipsum) + 1) != strlen(lipsum)) return bad;
                }

                {
                    Accessor::PacketStream reader(builder);
                    Accessor::PacketBuilder copy;
                    copy.init(Accessor::pool.allocateDirect<Accessor::Pool::Quota::Tx>(Accessor::bytesToBlocks(strlen(lipsum))));
                    if(copy.copyFrom(reader, strlen(lipsum) + 10) != strlen(lipsum)) return bad;
                }

                {
                    Accessor::PacketStream reader(builder);
                    Accessor::PacketBuilder copy;
                    copy.init(Accessor::pool.allocateDirect<Accessor::Pool::Quota::Tx>(1));
                    if(copy.copyFrom(reader, strlen(lipsum)) != Accessor::blockMaxPayload) return bad;
                }

                return ok;
            }
    } task;

    work(task);
}

namespace detailsOfStructuredAccessor {
	struct End {
		static constexpr auto offset = 8;
		static constexpr auto length = 0;

		template<class Stream> inline bool read(Stream& s) {
			return true;
		}
	};
}

TEST(NetPacket, StructuredAccessor) {
    struct Task: TaskBase<Task> {
    	struct X: Field8<0> {};
    	struct Y: Field16<1> {};
    	struct Z: Field32<3> {};
    	struct W: Field8<7> {};
    	using End = detailsOfStructuredAccessor::End;

    	bool run() {
			const uint8_t header[] = {0x50, 0x3e, 0xba, 0xad, 0x30, 0xda, 0xfa, 0xca};

			init(1);
			if(!builder.copyIn((const char*)header, sizeof(header))) return bad;
            builder.done();

            {
            	Accessor::PacketStream reader(builder);
            	StructuredAccessor<X, Y, Z, W> data;
            	if(!data.extract(reader)) return bad;
            	if(data.get<X>() != 0x50) return bad;
            	if(data.get<Y>() != 0x3eba) return bad;
            	if(data.get<Z>() != 0xad30dafa) return bad;
            	if(data.get<W>() != 0xca) return bad;
            }

            {
            	Accessor::PacketStream reader(builder);
            	StructuredAccessor<X, Z, W> data;
            	if(!data.extract(reader)) return bad;
            	if(data.get<X>() != 0x50) return bad;
            	if(data.get<Z>() != 0xad30dafa) return bad;
            	if(data.get<W>() != 0xca) return bad;
            }

            {
            	Accessor::PacketStream reader(builder);
            	StructuredAccessor<Y, W> data;
            	if(!data.extract(reader)) return bad;
            	if(data.get<Y>() != 0x3eba) return bad;
            	if(data.get<W>() != 0xca) return bad;
            }

            {
            	Accessor::PacketStream reader(builder);
            	StructuredAccessor<X, Z> data;
            	if(!data.extract(reader)) return bad;
            	if(data.get<X>() != 0x50) return bad;
            	if(data.get<Z>() != 0xad30dafa) return bad;
            }

            {
            	Accessor::PacketStream reader(builder);
            	StructuredAccessor<X> data1, data2, data3;
            	if(!data1.extract(reader)) return bad;
            	if(!data2.extract(reader)) return bad;
            	if(!data3.extract(reader)) return bad;
            	if(data1.get<X>() != 0x50) return bad;
            	if(data2.get<X>() != 0x3e) return bad;
            	if(data3.get<X>() != 0xba) return bad;
            }

            {
            	Accessor::PacketStream reader(builder);
            	StructuredAccessor<Y, End> data;
            	if(!data.extract(reader)) return bad;
            	if(data.get<Y>() != 0x3eba) return bad;

            	uint8_t temp;
            	if(reader.read8(temp)) return bad;
            }

            {
                Accessor::PacketBuilder builder;
                builder.init(Accessor::pool.allocateDirect<Accessor::Pool::Quota::Tx>(1));

                StructuredAccessor<X, Y, Z, W> data;
                data.get<X>() = 0x50;
                data.get<Y>() = 0x3eba;
                data.get<Z>() = 0xad30dafa;
                data.get<W>() = 0xca;
                if(!data.fill(builder)) return bad;
                builder.done();

                Accessor::PacketStream reader(builder);
                StructuredAccessor<X, Y, Z, W> check;
                if(!check.extract(reader)) return bad;
                if(check.get<X>() != 0x50) return bad;
                if(check.get<Y>() != 0x3eba) return bad;
                if(check.get<Z>() != 0xad30dafa) return bad;
                if(check.get<W>() != 0xca) return bad;
            }

            {
                Accessor::PacketBuilder builder;
                builder.init(Accessor::pool.allocateDirect<Accessor::Pool::Quota::Tx>(1));

                StructuredAccessor<Y, W> data;
                data.get<Y>() = 0x3eba;
                data.get<W>() = 0xca;
                if(!data.fill(builder)) return bad;
                builder.done();

                Accessor::PacketStream reader(builder);
                StructuredAccessor<Y, W> check;
                if(!check.extract(reader)) return bad;
                if(check.get<Y>() != 0x3eba) return bad;
                if(check.get<W>() != 0xca) return bad;

                Accessor::PacketStream modifier(builder);
                StructuredAccessor<X, Z> pass2;
                pass2.get<X>() = 0x50;
                pass2.get<Z>() = 0xad30dafa;
                if(!pass2.fill(modifier)) return bad;

                Accessor::PacketStream reader2(builder);
                StructuredAccessor<X, Y, Z, W> check2;
                if(!check2.extract(reader2)) return bad;
                if(check2.get<X>() != 0x50) return bad;
                if(check2.get<Y>() != 0x3eba) return bad;
                if(check2.get<Z>() != 0xad30dafa) return bad;
                if(check2.get<W>() != 0xca) return bad;
            }

			return ok;
		}
	} task;

	work(task);
}

namespace detailsOfStructuredAccessorReal {
	struct HeaderInfo: 	Field16<0> {
		template<class Stream> inline bool read(Stream& s) {
			if(!HeaderInfo::Field16::read(s))
				return false;

			if((data & 0xf000) != 0x4000)
				return false;

			s.start(((data & 0x0f00) >> 6) - 2);
			s.patchNet(data);
			return true;
		}
	};
}

TEST(NetPacket, StructuredAccessorReal) {
	struct Task: TaskBase<Task> {
    	struct Length: 		Field16<2> {};
    	struct FragId: 		Field16<4> {};
    	struct FlagOff: 	Field16<6> {};
    	struct Ttl: 		Field8<8> {};
    	struct Proto: 		Field8<9> {};
    	struct Sum: 		Field16<10> {};
    	struct Src: 		Field32<12> {};
    	struct Dst: 		Field32<16> {};
    	using HeaderInfo = detailsOfStructuredAccessorReal::HeaderInfo;

    	bool run() {
			const uint8_t header[] = {
		            /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
		            0x45, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfe, 0x11, 0xc8,
		            /* source IP address  | destination IP address */
		            0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
		            /* data */
		            'f', 'o', 'o', 'b', 'a', 'r'
			};

			init(1);
			if(!builder.copyIn((const char*)header, sizeof(header))) return bad;
            builder.done();

            {
            	Accessor::SummedPacketStream reader(builder);
            	StructuredAccessor<HeaderInfo, Length, Proto, Src> data;

            	if(!data.extract(reader)) return bad;
            	if(data.get<Length>() != 0x1a) return bad;
            	if(data.get<Proto>() != 0xfe) return bad;
            	if(data.get<Src>() != 0x0a0a0a01) return bad;

            	reader.finish();
            	if(reader.result()) return bad;

            	if(readAndCheck(reader, "foobar") != ok) return bad;
            }

			return ok;
		}
	} task;

	work(task);
}
