/*
 * NetPacket.cpp
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

static const char c55[] = "0123456789012345678901234567890123456789012345678901234";
static const char c57[] = "012345678901234567890123456789012345678901234567890123456";

TEST_GROUP(NetPacket) {

	template<class Child>
	struct TaskBase: TestTask<Child> {
		NetworkTestAccessor::PacketBuilder builder;

		void init(size_t n) {
			builder.init(NetworkTestAccessor::pool.allocateDirect(n));
		}

		bool readAndCheck(NetworkTestAccessor::PacketStream &reader, const char* str) {
			while(*str) {
				if(reader.atEop()) return TaskBase::TestTask::bad;
				if(reader.read8() != *str++) return TaskBase::TestTask::bad;
			}

			return TaskBase::TestTask::ok;
		}

		bool checkStreamContent(const char* str) {
			NetworkTestAccessor::PacketStream reader;
			reader.init(builder);
			return readAndCheck(reader, str);
		}

		bool checkDirectStreamContent(const char* str) {
			NetworkTestAccessor::PacketStream reader;
			reader.init(builder);

			while(*str) {
				Net::Chunk chunk = reader.getChunk();

				if(!chunk.length()) return TaskBase::TestTask::bad;
				if(*chunk.start != *str++) return TaskBase::TestTask::bad;

				reader.advance(1);
			}

			return TaskBase::TestTask::ok;
		}

		bool checkBlockContent(const char* str) {
			NetworkTestAccessor::PacketDisassembler disassembler;
			disassembler.init(builder);

			size_t length = strlen(str);

			do {
				Net::Chunk chunk = disassembler.getCurrentChunk();
				size_t runLength = chunk.length() < length ? chunk.length() : length;

				if(memcmp(chunk.start, str, runLength) != 0)
					return TaskBase::TestTask::bad;

				str += runLength;
				length -= runLength;
			}while(disassembler.advance());

			if(length) return TaskBase::TestTask::bad;

			return TaskBase::TestTask::ok;
		}
	};

	typedef NetworkTestAccessor::PacketBuilder::PacketWriterBase Writer;
	template<const char* padding, class Data, bool (Writer::* write)(Data), Data (NetworkTestAccessor::PacketStream::* read)()>
	struct OverlapTask: TaskBase<OverlapTask<padding, Data, write, read>> {
		static constexpr Data value1 = static_cast<Data>(0xb16b00b5);
		static constexpr Data value2 = static_cast<Data>(0x600dc0de);

		bool run() {
			this->init(2);
			if(this->builder.copyIn(padding, strlen(padding)) != strlen(padding)) return OverlapTask::bad;
			if(!(this->builder.*write)(value1)) return OverlapTask::bad;
			if(!(this->builder.*write)(value2)) return OverlapTask::bad;
			this->builder.done();

			NetworkTestAccessor::PacketStream reader;
			reader.init(this->builder);

			if(this->readAndCheck(reader, padding) == OverlapTask::bad)
				return OverlapTask::bad;

			if((reader.*read)() != value1) return OverlapTask::bad;
			if((reader.*read)() != value2) return OverlapTask::bad;

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
			init(2);
			if(NetworkTestAccessor::pool.statUsed() != 2) return bad;
			if(builder.copyIn(foobar, strlen(foobar)) != strlen(foobar)) return bad;

			builder.done();

			if(NetworkTestAccessor::pool.statUsed() != 1) return bad;

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

			NetworkTestAccessor::PacketStream reader;
			reader.init(builder);

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

			NetworkTestAccessor::PacketStream reader;
			reader.init(builder);

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
			&NetworkTestAccessor::PacketBuilder::write32net,
			&NetworkTestAccessor::PacketStream::read32net
	> task;
	work(task);
}

TEST(NetPacket, HalfWordOverlapNet) {
	OverlapTask<
			c57,
			uint16_t,
			&NetworkTestAccessor::PacketBuilder::write16net,
			&NetworkTestAccessor::PacketStream::read16net
	> task;
	work(task);
}

TEST(NetPacket, WordOverlapRawt) {
	OverlapTask<
			c55,
			uint32_t,
			&NetworkTestAccessor::PacketBuilder::write32raw,
			&NetworkTestAccessor::PacketStream::read32raw
	> task;
	work(task);
}

TEST(NetPacket, HalfWordOverlapRaw) {
	OverlapTask<
			c57,
			uint16_t,
			&NetworkTestAccessor::PacketBuilder::write16raw,
			&NetworkTestAccessor::PacketStream::read16raw
	> task;
	work(task);
}
TEST(NetPacket, SimplePatch) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(hello, strlen(hello)) != strlen(hello)) return bad;
			builder.done();

			NetworkTestAccessor::PacketStream modifier;
			modifier.init(builder);

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

			NetworkTestAccessor::PacketStream modifier;
			modifier.init(builder);

			if(modifier.read8() != 123) return bad;

			for(int i=0; i<9; i++) {
				if(modifier.read32net() != (3u * i)) return bad;
				if(!modifier.write32net(3 * i)) return bad;
				if(!modifier.skipAhead(4)) return bad;
			}

			if(modifier.skipAhead(4)) return bad;

			NetworkTestAccessor::PacketStream reader;
			reader.init(builder);

			if(reader.read8() != 123) return bad;

			for(int i=0; i<9; i++) {
				if(reader.read32net() != (3u * i)) return bad;
				if(reader.read32net() != (3u * i)) return bad;
				if(reader.read32net() != (3u * i + 2u)) return bad;
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
            if(!builder.addByReference(world, strlen(hello))) return bad;
            if(!builder.write8('|')) return bad;
            builder.done();

            return checkStreamContent("|hello|world|");
        }
    } task;

    work(task);
}

