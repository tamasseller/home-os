/*
 * TcpEcho.cpp
 *
 *  Created on: 2018.02.25.
 *      Author: tooma
 */

#include "System.h"

class TcpEchoTask: Os::Task {
	friend class Os::Task;
	size_t stack[64*1024*1024];

	Net::TcpListener listener;
	Net::TcpSocket socket;

	void run() {
		listener.init();
		socket.init();

        listener.listen(1234);

        while(true) {
            listener.wait();

            Os::assert(listener.accept(socket), "WTF?");

            while(socket.isOk()) {
            	socket.getRx().wait();

                Net::Chunk c = socket.getRx().getChunk();

                // TODO do something with the data.

                socket.getRx().advance((uint16_t)c.length);
            }

            socket.abandon();
        }
	}

public:
	void start() {
		Os::Task::start<TcpEchoTask, &TcpEchoTask::run>(&stack, sizeof(stack));
	}
} tcpEchoTask;

void startTcpEchoTask() {
	tcpEchoTask.start();
}
