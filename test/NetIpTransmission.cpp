/*
 * NetIpTransmitter.cpp
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

TEST_GROUP(NetIpTransmitter) {
    template<class Net>
    struct TxTests {
            using Accessor = NetworkTestAccessor<Net>;

            static void expectArpReq(size_t n) {
                Net::template geEthernetInterface<DummyIf>()->expectN(n,
                    /*            dst                 |                src                | etherType */
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x06,
                    /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                    0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00,
                    /*      sender IP     |          target MAC               |       target IP       */
                    0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x01
                );
            }

            static void expectIp() {
                Net::template geEthernetInterface<DummyIf>()->expectN(1,
                    /*            dst                 |                src                | etherType */
                    0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                    /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                    0x45, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfe, 0x11, 0xc8,
                    /* source IP address  | destination IP address */
                    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
                    /* TBD */
                    'f', 'o', 'o', 'b', 'a', 'r'
                );
            }

            static void expectLongIp() {
                Net::template geEthernetInterface<DummyIf>()->expectN(1,
                        /*            dst                 |                src                | etherType */
                        0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                        /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                        0x45, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfe, 0x11, 0xa3,
                        /* source IP address  | destination IP address */
                        0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
                        /* TBD */
                        'T', 'h', 'e', ' ', 'q', 'u', 'i', 'c', 'k', ' ', 'b', 'r', 'o', 'w', 'n', ' ',
                        'f', 'o', 'x', ' ', 'j', 'u', 'm', 'p', 's', ' ', 'o', 'v', 'e', 'r', ' ', 't',
                        'h', 'e', ' ', 'l', 'a', 'z', 'y', ' ', 'd', 'o', 'g'
                    );
                }

            template<bool addRoute, bool addArp, class Task>
            static inline void work(Task& task)
            {
                task.start();
                Net::init(NetBuffers<Net>::buffers);

                if(addRoute) {
                    Net::addRoute(
                    		typename Net::Route(
                    			Net::template geEthernetInterface<DummyIf>(),
                    			AddressIp4::make(10, 10, 10, 10),
                    			8
							),
                    		true
                    );
                }

                if(addArp) {
                    Net::template geEthernetInterface<DummyIf>()->getArpCache()->set(
                            AddressIp4::make(10, 10, 10, 1),
                            AddressEthernet::make(0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E),
                            32767
                    );
                }

                CommonTestUtils::start();
                CHECK(!task.error);
            }

            static inline void runNoRoute() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::IpTransmitter tx;
                        tx.init();
                        if(tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        if(tx.getError() != NetErrorStrings::noRoute)
                            return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<false, false>(task);
            }

            static inline void runUnresolved() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::IpTransmitter tx;
                        tx.init();

                        expectArpReq(4);

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        tx.wait();

                        if(tx.getError() != NetErrorStrings::unresolved)
                            return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, false>(task);
            }

            static inline void runResolved() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::IpTransmitter tx;
                        tx.init();

                        expectArpReq(1);

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        if(!tx.isOccupied())
                            return Task::bad;

                        Net::template geEthernetInterface<DummyIf>()->getArpCache()->set(
                                AddressIp4::make(10, 10, 10, 1),
                                AddressEthernet::make(0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E),
                                32768
                        );

                        tx.wait();

                        if(tx.getError())
                            return Task::bad;

                        if(tx.fill("foobar", 6) != 6)
                            return Task::bad;

                        expectIp();

                        if(!tx.send(254))
                            return Task::bad;

                        tx.wait();
                        return Task::ok;
                    }
                } task;

                work<true, false>(task);
            }

            static inline void runSuccessful() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::IpTransmitter tx;
                        tx.init();
                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        if(tx.isOccupied() || tx.getError())
                            return Task::bad;

                        if(tx.fill("foobar", 6) != 6)
                            return Task::bad;

                        expectIp();

                        if(!tx.send(254))
                            return Task::bad;

                        tx.wait();

                        if(Accessor::pool.statUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runLonger() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::IpTransmitter tx;
                        static constexpr const char* text = "The quick brown fox jumps over the lazy dog";

                        tx.init();
                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), strlen(text)))
                            return Task::bad;

                        if(tx.isOccupied() || tx.getError())
                            return Task::bad;

                        if(tx.fill(text, strlen(text)) != strlen(text))
                            return Task::bad;

                        expectLongIp();

                        if(!tx.send(254))
                            return Task::bad;

                        tx.wait();

                        if(Accessor::pool.statUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runWaitForBuffers() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::IpTransmitter tx1, tx2;

                        tx1.init();
                        tx2.init();

                        static constexpr auto nBytesAvailable = tx1.blockMaxPayload * (64 * 75 / 100);

                        if(!tx1.prepare(AddressIp4::make(10, 10, 10, 1), nBytesAvailable - 14 - 20))
                            return Task::bad;

                        if(tx1.isOccupied() || tx1.getError())
                            return Task::bad;

                        if(tx1.fill("foobar", 6) != 6)
                            return Task::bad;

                        if(!tx2.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        if(!tx2.isOccupied())
                            return Task::bad;

                        expectIp();

                        if(!tx1.send(254))
                            return Task::bad;

                        tx1.wait();

                        tx2.wait();

                        if(tx2.isOccupied() || tx2.getError())
                            return Task::bad;

                        if(tx2.fill("foobar", 6) != 6)
                            return Task::bad;

                        expectIp();

                        if(!tx2.send(254))
                            return Task::bad;

                        tx2.wait();

                        if(Accessor::pool.statUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runMulti() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::IpTransmitter tx;
                        static constexpr const char* text = "The quick brown fox jumps over the lazy dog";

                        tx.init();

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        if(tx.fill("foobar", 6) != 6)
                            return Task::bad;

                        expectIp();
                        tx.send(254);

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), strlen(text)))
                            return Task::bad;

                        if(tx.fill(text, strlen(text)) != strlen(text))
                            return Task::bad;

                        expectLongIp();

                        if(!tx.send(254))
                            return Task::bad;

                        tx.wait();

                        if(Accessor::pool.statUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runIndirect() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::IpTransmitter tx;
                        static constexpr const char* text = "The quick brown fox jumps over the lazy dog";

                        tx.init();

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 0, 1))
                            return Task::bad;

                        if(!tx.addIndirect(text, strlen(text)))
                            return Task::bad;

                        expectLongIp();

                        if(!tx.send(254))
                            return Task::bad;

                        tx.wait();

                        if(Accessor::pool.statUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runIndirectWithDestructor() {
                static constexpr const char* text = "The quick brown fox jumps over the lazy dog";

                struct Task: public TestTask<Task> {
                    bool destroyed = false;

                    static void destroy(void* ptr, const char* data, uint16_t length) {
                        auto *self = reinterpret_cast<Task*>(ptr);

                        if(data == text && length == strlen(text))
                            self->destroyed = true;
                    }

                    bool run() {
                        typename Net::IpTransmitter tx;

                        tx.init();

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 0, 1))
                            return Task::bad;

                        if(!tx.addIndirect(text, strlen(text), &Task::destroy, this))
                            return Task::bad;

                        expectLongIp();

                        if(!tx.send(254))
                            return Task::bad;

                        tx.wait();

                        if(Accessor::pool.statUsed()) return Task::bad;
                        if(!destroyed) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }
    };
};

#define INSTANTIATE_ALL_TESTS(x)                            \
                                                            \
TEST(NetIpTransmitter, NoRoute##x) {                        \
    TxTests<Net##x>::runNoRoute();                          \
}                                                           \
                                                            \
TEST(NetIpTransmitter, Successful##x) {                     \
    TxTests<Net##x>::runSuccessful();                       \
}                                                           \
                                                            \
TEST(NetIpTransmitter, Longer##x) {                         \
    TxTests<Net##x>::runLonger();                           \
}                                                           \
															\
TEST(NetIpTransmitter, WaitForBuffers##x) {                 \
    TxTests<Net##x>::runWaitForBuffers();                   \
}                                                           \
                                                            \
TEST(NetIpTransmitter, Multi##x) {                          \
    TxTests<Net##x>::runMulti();                            \
}                                                           \
                                                            \
TEST(NetIpTransmitter, Indirect##x) {                       \
    TxTests<Net##x>::runIndirect();                         \
}                                                           \
                                                            \
TEST(NetIpTransmitter, IndirectWithDestructor##x) {         \
    TxTests<Net##x>::runIndirectWithDestructor();           \
}                                                           \
															\
TEST(NetIpTransmitter, Unresolved##x) {                     \
    TxTests<Net##x>::runUnresolved();                       \
}                                                           \
                                                            \
TEST(NetIpTransmitter, Resolved##x) {                       \
    TxTests<Net##x>::runResolved();                         \
}

INSTANTIATE_ALL_TESTS(64)
INSTANTIATE_ALL_TESTS(43)
