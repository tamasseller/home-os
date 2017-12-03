/*
 * NetChecksum.cpp
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

#include "InetChecksumDigester.h"

TEST_GROUP(NetChecksum) {};

TEST(NetChecksum, RFC1071) {
	constexpr static const char* rfc1071 =
		"\x00\x01\xf2\x03\xf4\xf5\xf6\xf7\x00"
		"\x00\x01\xf2\x03\xf4\xf5\xf6\xf7\x00"
		"\x00\x01\xf2\x03\xf4\xf5\xf6\xf7\x00"
		"\x00\x01\xf2\x03\xf4\xf5\xf6\xf7";

	uint16_t expectedSum;
	reinterpret_cast<uint8_t*>(&expectedSum)[0] = 0xdd;
	reinterpret_cast<uint8_t*>(&expectedSum)[1] = 0xf2;

	for(int i=0; i<4; i++) {
		for(int j=0; j<8; j++) {
			InetChecksumDigester c;
			c.consume(rfc1071 + i * 9, j);
			c.consume(rfc1071 + i * 9 + j, 8 - j, j & 1);
			CHECK(static_cast<uint16_t>(~c.result()) == expectedSum);
		}
	}
}

TEST(NetChecksum, Alignment) {
	const char* data = "aabaabaab";

	uint16_t results[4];
	for(int i=0; i<4; i++) {
		InetChecksumDigester c;
		c.consume(data + i, 6);
		results[i] = c.result();
	}

	CHECK(results[0] == results[1]);
	CHECK(results[1] == results[2]);
	CHECK(results[2] == results[3]);
}

TEST(NetChecksum, Realworld) {
	char data[] = "\x45\x00\x00\x3c\xf5\xdf\x40\x00\x40\x06\x00\x00\xc0\xa8\x01\x07\x0a\x0a\x0a\x01";;

	InetChecksumDigester generator;
	generator.consume(data, sizeof(data));

	uint16_t chks = generator.result();
	memcpy(data + 10, &chks, 2);

	InetChecksumDigester checker;
	checker.consume(data, sizeof(data));

	CHECK(checker.result() == 0);
}

TEST(NetChecksum, Patch) {
	char data[] alignas(uint16_t) = "\x45\x00\x00\x3c\xf5\xdf\x40\x00\x40\x06\x00\x00\xc0\xa8\x01\x07\x0a\x0a\x0a\x01";
	InetChecksumDigester generator;
	generator.consume(data, sizeof(data));

	uint16_t old = reinterpret_cast<uint16_t*>(data)[1];
	reinterpret_cast<uint16_t*>(data)[1] = 0x1234;
	generator.patch(old, 0x1234);

	uint16_t chks = generator.result();
	memcpy(data + 10, &chks, 2);

	InetChecksumDigester checker;
	checker.consume(data, sizeof(data));
	CHECK(checker.result() == 0);
}

TEST(NetChecksum, Higher) {
	char data[] alignas(uint16_t) = "\x45\x00\x00\x3c\xf5\xdf\x40\x00\x40\x06\x00\x00\xc0\xa8\x01\x07\x0a\x0a\x0a\x01";
	InetChecksumDigester generator;
	generator.consume(data, sizeof(data));

	uint16_t old = reinterpret_cast<uint16_t*>(data)[1];
	reinterpret_cast<uint16_t*>(data)[1] = 0xffff;
	generator.patch(old, 0xffff);

	uint16_t chks = generator.result();
	memcpy(data + 10, &chks, 2);

	InetChecksumDigester checker;
	checker.consume(data, sizeof(data));
	CHECK(checker.result() == 0);
}
