/*
 * UdpEcho.cpp
 *
 *  Created on: 2018.02.25.
 *      Author: tooma
 */

#include "System.h"

class UdpEchoTask: Os::Task {
	friend class Os::Task;
	size_t stack[64*1024*1024];

	Net::UdpReceiver rx;
	Net::UdpTransmitter tx;

	void run() {
		rx.init();
        rx.receive(1234);
        tx.init(1234);

        while(true) {
            rx.wait();

           	tx.prepare(rx.getPeerAddress(), rx.getPeerPort(), rx.getLength());
           	Os::assert(!tx.getError(), tx.getError());

            while(!rx.atEop()) {
                Net::Chunk c = rx.getChunk();
                Os::assert(tx.fill(c.start, (uint16_t)c.length), "WTF?");
                rx.advance((uint16_t)c.length);
            }

            tx.send();
        }
	}

public:
	void start() {
		Os::Task::start<UdpEchoTask, &UdpEchoTask::run>(&stack, sizeof(stack));
	}
} udpEchoTask;

void startUdpEchoTask() {
	udpEchoTask.start();
}
