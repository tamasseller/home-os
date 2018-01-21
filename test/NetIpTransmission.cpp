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
                Net::template getEthernetInterface<DummyIf>()->expectN(n,
                    /*            dst                 |                src                | etherType */
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x06,
                    /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                    0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00,
                    /*      sender IP     |          target MAC               |       target IP       */
                    0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x01
                );
            }

            static void expectIp(size_t n = 1) {
                Net::template getEthernetInterface<DummyIf>()->expectN(n,
                    /*            dst                 |                src                | etherType */
                    0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                    /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                    0x45, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfe, 0x11, 0xc8,
                    /* source IP address  | destination IP address */
                    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
                    /* data */
                    'f', 'o', 'o', 'b', 'a', 'r'
                );
            }

            static void expectIp1234(size_t n = 1) {
                Net::template getEthernetInterface<DummyIf>()->expectN(n,
                    /*            dst                 |                src                | etherType */
                    0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                    /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                    0x45, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfe, 0x21, 0xcd,
                    /* source IP address  | destination IP address */
                    0x0a, 0x0a, 0x0a, 0x0a, 0x01, 0x02, 0x03, 0x04,
                    /* data */
                    'f', 'o', 'o', 'b', 'a', 'r'
                );
            }

            static void expectLongIp() {
                Net::template getEthernetInterface<DummyIf>()->expectN(1,
                        /*            dst                 |                src                | etherType */
                        0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                        /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                        0x45, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfe, 0x11, 0xa3,
                        /* source IP address  | destination IP address */
                        0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
                        /* data */
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
                    			Net::template getEthernetInterface<DummyIf>(),
                    			AddressIp4::make(10, 10, 10, 10),
                    			8
							),
                    		true
                    );

                    // Default route
                    Net::addRoute(
                            typename Net::Route(
                                Net::template getEthernetInterface<DummyIf>(),
                                AddressIp4::make(0, 0, 0, 0),       // Destination
                                0,                                  // Mask
                                AddressIp4::make(10, 10, 10, 1),    // Gateway router
                                AddressIp4::make(10, 10, 10, 10)    // Source address
                            ),
                            true
                    );
                }

                if(addArp) {
                    Net::template getEthernetInterface<DummyIf>()->getArpCache()->set(
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
                        typename Net::RawTransmitter tx;
                        tx.init();
                        if(tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        if(tx.getError() != NetErrorStrings::noRoute)
                            return Task::bad;

                        if(tx.fill("foobar", 6) != 0)
                            return Task::bad;

                        static constexpr const char* text = "whatever";
                        if(tx.addIndirect(text, strlen(text)))
                            return Task::bad;

                        if(tx.send(254))
                            return Task::bad;

                        if(Net::getCounterStats().ip.outputNoRoute != 1) return Task::bad;
                        if(Net::getCounterStats().ip.outputArpFailed != 0) return Task::bad;
                        if(Net::getCounterStats().ip.outputRequest != 1) return Task::bad;
                        if(Net::getCounterStats().ip.outputQueued != 0) return Task::bad;
                        if(Net::getCounterStats().ip.outputSent != 0) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<false, false>(task);
            }

            static inline void runUnresolved() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
                        tx.init();

                        expectArpReq(4);

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        tx.wait();

                        if(tx.getError() != NetErrorStrings::unresolved)
                            return Task::bad;

                        if(Net::getCounterStats().arp.outputQueued != 4) return Task::bad;
                        if(Net::getCounterStats().arp.outputSent != 4) return Task::bad;
                        if(Net::getCounterStats().arp.requestSent != 4) return Task::bad;
                        if(Net::getCounterStats().ip.outputNoRoute != 0) return Task::bad;
                        if(Net::getCounterStats().ip.outputArpFailed != 1) return Task::bad;
                        if(Net::getCounterStats().ip.outputRequest != 1) return Task::bad;
                        if(Net::getCounterStats().ip.outputQueued != 0) return Task::bad;
                        if(Net::getCounterStats().ip.outputSent != 0) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, false>(task);
            }

            static inline void runResolvedByHand() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
                        tx.init();

                        expectArpReq(1);

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        if(!tx.isOccupied())
                            return Task::bad;

                        Net::template getEthernetInterface<DummyIf>()->getArpCache()->set(
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

            static inline void runResolvedByRx() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        auto initialRxUsage = Accessor::pool.statRxUsed();

                        typename Net::RawTransmitter tx;
                        tx.init();

                        expectArpReq(1);

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        if(Net::getCounterStats().ip.outputRequest != 1) return Task::bad;

                        if(!tx.isOccupied())
                            return Task::bad;

                        /*
                         * Multiple identical replies to increase code coverage.
                         */
                        Task::doIndirect([](){
                        	for(int i=0; i<3; i++) {
								Net::template getEthernetInterface<DummyIf>()->receive(
									/*            dst                 |                src                | etherType */
									0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
									/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
									0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
									/*      sender IP     |          target MAC               |       target IP       */
									0x0a, 0x0a, 0x0a, 0x01, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x0a, 0x0a, 0x0a, 0x0a
								);
                        	}
                        });

                        tx.wait();

                        if(tx.getError())
                            return Task::bad;

                        if(tx.fill("foobar", 6) != 6)
                            return Task::bad;

                        expectIp();

                        if(!tx.send(254))
                            return Task::bad;

                        if(Net::getCounterStats().ip.outputQueued != 1) return Task::bad;

                        tx.wait();

                        if(Net::getCounterStats().ip.outputSent != 1) return Task::bad;

                        if(Accessor::pool.statTxUsed()) return Task::bad;
                        if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
                        if(Net::getCounterStats().arp.inputReceived != 3) return Task::bad;
                        if(Net::getCounterStats().arp.outputSent != 1) return Task::bad;
                        if(Net::getCounterStats().arp.outputQueued != 1) return Task::bad;
                        if(Net::getCounterStats().arp.requestSent != 1) return Task::bad;
                        if(Net::getCounterStats().arp.replyReceived != 3) return Task::bad;
                        if(Net::getCounterStats().ip.outputRequest != 1) return Task::bad;
                        if(Net::getCounterStats().ip.outputQueued != 1) return Task::bad;
                        if(Net::getCounterStats().ip.outputSent != 1) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, false>(task);
            }

            static inline void runSuccessful() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
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

                        if(Accessor::pool.statTxUsed()) return Task::bad;
                        if(Net::getCounterStats().arp.inputReceived != 0) return Task::bad;
                        if(Net::getCounterStats().arp.outputSent != 0) return Task::bad;
                        if(Net::getCounterStats().arp.outputQueued != 0) return Task::bad;
                        if(Net::getCounterStats().arp.requestSent != 0) return Task::bad;
                        if(Net::getCounterStats().arp.replyReceived != 0) return Task::bad;
                        if(Net::getCounterStats().ip.outputRequest != 1) return Task::bad;
                        if(Net::getCounterStats().ip.outputQueued != 1) return Task::bad;
                        if(Net::getCounterStats().ip.outputSent != 1) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runThroughDefaultRoute() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
                        tx.init();
                        if(!tx.prepare(AddressIp4::make(1, 2, 3, 4), 6))
                            return Task::bad;

                        if(tx.isOccupied() || tx.getError())
                            return Task::bad;

                        if(tx.fill("foobar", 6) != 6)
                            return Task::bad;

                        expectIp1234();

                        if(!tx.send(254))
                            return Task::bad;

                        tx.wait();

                        if(Accessor::pool.statTxUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }


            static inline void runLonger() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
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

                        if(Accessor::pool.statTxUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runWaitForBuffers() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx1, tx2;

                        tx1.init();
                        tx2.init();

                        static constexpr auto nBytesAvailable = Net::blockMaxPayload * (64 * 75 / 100);

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

                        if(tx2.fill("foobar", 6) != 6)
                            return Task::bad;

                        expectIp();

                        if(!tx2.send(254))
                            return Task::bad;

                        tx2.wait();

                        if(Accessor::pool.statTxUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

			static inline void runMulti() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
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

                        if(Accessor::pool.statTxUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runIndirect() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
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

                        if(Accessor::pool.statTxUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runIndirectWithDestructor() {
                static constexpr const char* text = "The quick brown fox jumps over the lazy dog";

                struct Task: public TestTask<Task> {
                    bool destroyed = false;

                    static void destroy(void* ptr, const char* data, uint32_t length) {
                        auto *self = reinterpret_cast<Task*>(ptr);

                        if(data == text && length == strlen(text))
                            self->destroyed = true;
                    }

                    bool run() {
                        typename Net::RawTransmitter tx;

                        tx.init();

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 0, 1))
                            return Task::bad;

                        if(!tx.addIndirect(text, strlen(text), &Task::destroy, this))
                            return Task::bad;

                        expectLongIp();

                        if(!tx.send(254))
                            return Task::bad;

                        tx.wait();

                        if(Accessor::pool.statTxUsed()) return Task::bad;
                        if(!destroyed) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runPrefetch() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter txs[3];

                        Net::template getEthernetInterface<DummyIf>()->setDelayed(true);

                        for(auto i=0u; i<sizeof(txs)/sizeof(txs[0]); i++) {
                        	auto &tx = txs[i];
                            tx.init();

                            if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 0, 1))
                                return Task::bad;

                            if(tx.fill("foobar", 6) != 6)
                                return Task::bad;

                            if(!tx.send(254))
                                return Task::bad;
                        }

                        txs[0].cancel(); // Can not be canceled.

                        expectIp(3);
                        Net::template getEthernetInterface<DummyIf>()->setDelayed(false);

                        for(auto i=0u; i<sizeof(txs)/sizeof(txs[0]); i++)
                        	txs[i].wait();

                        if(Accessor::pool.statTxUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runFunnyHeaders() {
                struct Task: public TestTask<Task> {
                    bool run() {
                    	typename Net::RawTransmitter tx;
                    	tx.init();

                    	uint16_t sizes[4] = {
                    			Net::blockMaxPayload - 1,
                    			Net::blockMaxPayload + 1,
                    			2 * Net::blockMaxPayload - 1,
                    			2 * Net::blockMaxPayload + 1
                    	};

                    	for(uint16_t headerSize: sizes) {
                    		Accessor::overrideHeaderSize(Net::template getEthernetInterface<DummyIf>(), headerSize);
                    		Net::template getEthernetInterface<DummyIf>()->setFillSize(headerSize);

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

                            if(Accessor::pool.statTxUsed()) return Task::bad;
                    	}

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);

            }

            static inline void runCancelInAllocation() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        AddressIp4 targets[2] = {
                             AddressIp4::make(10, 10, 10, 1),   // Has static ARP entry.
                             AddressIp4::make(10, 10, 10, 2)   // No ARP entry, triggers cancel in ARP request buffer allocation.
                        };
                        for(AddressIp4 target: targets) {
                            typename Net::RawTransmitter tx1, tx2;

                            tx1.init();
                            tx2.init();

                            static constexpr auto nBytesAvailable = Net::blockMaxPayload * (64 * 75 / 100);

                            if(!tx1.prepare(AddressIp4::make(10, 10, 10, 1), nBytesAvailable - 14 - 20))
                                return Task::bad;

                            if(tx1.fill("foobar", 6) != 6)
                                return Task::bad;

                            if(!tx2.prepare(target, 6))
                                return Task::bad;

                            tx2.cancel();
                            tx2.wait();

                            if(tx2.isOccupied() || tx2.getError() != NetErrorStrings::genericCancel)
                                return Task::bad;

                            if(tx2.sendTimeout(123, 254))
                                return Task::bad;

                            expectIp();

                            if(!tx1.send(254))
                                return Task::bad;

                            tx1.wait();

                            if(Accessor::pool.statTxUsed()) return Task::bad;
                        }

                        return Task::ok;
                    }
                } task;

                work<true, true>(task);
            }

            static inline void runTimeoutInArpRx() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
                        tx.init();

                        expectArpReq(1);

                        if(!tx.prepareTimeout(10, AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        tx.wait();

                        if(tx.getError() != NetErrorStrings::genericTimeout)
                            return Task::bad;

                        if(Accessor::pool.statTxUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, false>(task);
            }

            static inline void runCancelInArpTx() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter tx;
                        tx.init();

                        expectArpReq(1);

                        if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
                            return Task::bad;

                        tx.cancel();

                        tx.wait();

                        if(tx.getError() != NetErrorStrings::genericCancel)
                            return Task::bad;

                        if(Accessor::pool.statTxUsed()) return Task::bad;

                        return Task::ok;
                    }
                } task;

                work<true, false>(task);
            }

            static inline void runTimeoutInSend() {
                struct Task: public TestTask<Task> {
                    bool run() {
                        typename Net::RawTransmitter txs[3];

                        Net::template getEthernetInterface<DummyIf>()->setDelayed(true);

                        for(int i=0; i<3; i++) {
                            txs[i].init();

                            if(!txs[i].prepare(AddressIp4::make(10, 10, 10, 1), 6))
                                return Task::bad;

                            if(txs[i].fill("foobar", 6) != 6)
                                return Task::bad;
                        }

                        for(int i=0; i<2; i++)
                            if(!txs[i].send(254))
                                return Task::bad;

                        if(!txs[2].sendTimeout(1, 254))
                            return Task::bad;

                        txs[2].wait();

                        if(txs[2].getError() != NetErrorStrings::genericTimeout)
                            return Task::bad;

                        expectIp(2);

                        Net::template getEthernetInterface<DummyIf>()->setDelayed(false);

                        for(int i=0; i<2; i++)
                            txs[i].wait();

                        if(Accessor::pool.statTxUsed()) return Task::bad;

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
TEST(NetIpTransmitter, ThroughDefaultRoute##x) {            \
    TxTests<Net##x>::runThroughDefaultRoute();              \
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
TEST(NetIpTransmitter, ResolvedByHand##x) {                 \
    TxTests<Net##x>::runResolvedByHand();                   \
}														    \
															\
TEST(NetIpTransmitter, ResolvedByRx##x) {                   \
	TxTests<Net##x>::runResolvedByRx();                     \
}														    \
															\
TEST(NetIpTransmitter, Prefetch##x) {                       \
	TxTests<Net##x>::runPrefetch();                         \
}															\
    														\
TEST(NetIpTransmitter, FunnyHeaders##x) {                   \
	TxTests<Net##x>::runFunnyHeaders();                     \
}                                                           \
                                                            \
TEST(NetIpTransmitter, CancelInAllocation##x) {             \
    TxTests<Net##x>::runCancelInAllocation();               \
}                                                           \
                                                            \
TEST(NetIpTransmitter, TimeoutInArpRx##x) {                 \
    TxTests<Net##x>::runTimeoutInArpRx();                   \
}                                                           \
                                                            \
TEST(NetIpTransmitter, CancelInArpTx##x) {                  \
    TxTests<Net##x>::runCancelInArpTx();                    \
}                                                           \
                                                            \
TEST(NetIpTransmitter, TimeoutInSend##x) {                  \
    TxTests<Net##x>::runTimeoutInSend();                    \
}                                                           \

INSTANTIATE_ALL_TESTS(64)
INSTANTIATE_ALL_TESTS(43)
