/*
 * NetPacket.cpp
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

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
	static constexpr const char* c55 = "0123456789012345678901234567890123456789012345678901234";
	static constexpr const char* c57 = "012345678901234567890123456789012345678901234567890123456";
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

TEST(NetPacket, WordOverlap) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(c55, strlen(c55)) != strlen(c55)) return bad;
			if(!builder.write32(0xb16b00b5)) return bad;
			if(!builder.write32(0x600dc0de)) return bad;
			builder.done();

			NetworkTestAccessor::PacketStream reader;
			reader.init(builder);

			if(readAndCheck(reader, c55) == bad)
				return bad;

			if(reader.read32() != 0xb16b00b5) return bad;
			if(reader.read32() != 0x600dc0de) return bad;

			return ok;
		}
	} task;

	work(task);
}

TEST(NetPacket, HalfWordOverlap) {
	struct Task: TaskBase<Task> {
		bool run() {
			init(2);
			if(builder.copyIn(c57, strlen(c57)) != strlen(c57)) return bad;
			if(!builder.write16(0xb16b)) return bad;
			if(!builder.write16(0x00b5)) return bad;
			builder.done();

			NetworkTestAccessor::PacketStream reader;
			reader.init(builder);

			if(readAndCheck(reader, c57) == bad)
				return bad;

			if(reader.read16() != 0xb16b) return bad;
			if(reader.read16() != 0x00b5) return bad;

			return ok;
		}
	} task;

	work(task);
}

