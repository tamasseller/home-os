/*
 * main.cpp
 *
 *  Created on: 2018.01.04.
 *      Author: tooma
 */

#include "System.h"

#include <iostream>

Net::Buffers packetBuffers;

class UdpEchoTask: Os::Task {
	friend class Os::Task;
	size_t stack[1024*1024];

	Net::UdpReceiver rx;
	Net::UdpTransmitter tx;

	void run() {
		rx.init();
        rx.receive(1234);
        tx.init(1234);

        while(true) {
            rx.wait();
            tx.prepare(rx.getPeerAddress(), rx.getPeerPort(), rx.getLength());

            while(!rx.atEop()) {
                Net::Chunk c = rx.getChunk();
                tx.fill(c.start, (uint16_t)c.length());
                rx.advance((uint16_t)c.length());
            }

            tx.send();
        }
	}

public:
	void start() {
		Os::Task::start<UdpEchoTask, &UdpEchoTask::run>(&stack, sizeof(stack));
	}
} udpEchoTask;

class MainTask: Os::Task
{
	friend class Os::Task;
	size_t stack[1024*1024];

	void run() {
		printf("asd\n");
		Net::init(packetBuffers);
		Net::addRoute(Net::Route(
				Net::getEthernetInterface<LinuxTapDevice>(), AddressIp4::make(172, 23, 45, 67), 12), true);

		Net::template getEthernetInterface<LinuxTapDevice>()->setTapPeerAddress("172.23.45.1");

		udpEchoTask.start();

		Os::sleep(5 * 60 * 1000);
	}

public:
	void start() {
		Os::Task::start<MainTask, &MainTask::run>(&stack, sizeof(stack));
	}
} mainTask;

int main()
{
	mainTask.start();

	const char* ret = Os::start(1000);

	if(ret) {
		std::cerr << "Exited with error message:\n\n\t " << ret << "!\n\n";
		return 0;
	} else {
		std::cout << "Completed normally." << std::endl;
		return -1;
	}
}
