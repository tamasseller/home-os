/*
 * main.cpp
 *
 *  Created on: 2018.01.04.
 *      Author: tooma
 */

#include "System.h"

#include <iostream>
#include <sstream>

Net::Buffers packetBuffers;

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
                Os::assert(tx.fill(c.start, (uint16_t)c.length()), "WTF?");
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
	size_t stack[64*1024*1024];

	void run() {
		Net::init(packetBuffers);
		Net::addRoute(Net::Route(
				Net::getEthernetInterface<LinuxTapDevice>(), AddressIp4::make(172, 23, 45, 67), 12), true);

		Net::template getEthernetInterface<LinuxTapDevice>()->setTapPeerAddress("172.23.45.1");

		udpEchoTask.start();

		for(int i=0; i<5 * 60 * 1000; i++) {
			std::stringstream ss;

			auto bufferStats = Net::getBufferStats();
			auto stats = Net::getCounterStats();

			ss << std::endl << "Buffers used:\t" << bufferStats.nBuffersUsed << std::endl;
			ss << "\tRX:\t" << bufferStats.nRxUsed << std::endl;
			ss << "\tTX:\t" << bufferStats.nTxUsed << std::endl;

			ss << std::endl << "ARP stats: " << std::endl;

			ss << "\tReceived:\t\t" << stats.arp.inputReceived << std::endl;
			ss << "\t\tRequests:\t" << stats.arp.requestReceived << std::endl;
			ss << "\t\tReplies:\t" << stats.arp.replyReceived << std::endl;
			ss << "\t\tError:\t" << stats.arp.inputFormatError << std::endl;

			ss << "\tTx queued:\t\t" << stats.arp.outputQueued << std::endl;
			ss << "\t\tRequests:\t" << stats.arp.requestSent << std::endl;
			ss << "\t\tReplies:\t" << stats.arp.replySent << std::endl;
			ss << "\t\tWaiting:\t" << stats.arp.getTxWaiting() << std::endl;

			ss << std::endl << "IP stats: " << std::endl;

			ss << "\tReceived:\t\t" << stats.ip.inputReceived << std::endl;
			ss << "\t\tBad format:\t" << stats.ip.inputFormatError << std::endl;
			ss << "\t\tReassembly:\t" << stats.ip.inputReassemblyRequired << std::endl;
			ss << "\t\tForwarding:\t" << stats.ip.inputForwardingRequired << std::endl;

			ss << "\tPrepared:\t\t" << stats.ip.outputRequest << std::endl;
			ss << "\tTx queued:\t\t" << stats.ip.outputQueued << std::endl;
			ss << "\t\tNo route:\t" << stats.ip.outputNoRoute << std::endl;
			ss << "\t\tArp fail:\t" << stats.ip.outputArpFailed << std::endl;
			ss << "\t\tWaiting:\t" << stats.ip.getTxWaiting() << std::endl;

			ss << std::endl << "ICMP stats: " << std::endl;

			ss << "\tReceived:\t\t" << stats.icmp.inputReceived << std::endl;
			ss << "\t\tPing reqs:\t" << stats.icmp.pingRequests << std::endl;
			ss << "\t\tBad format:\t" << stats.icmp.inputFormatError << std::endl;
			ss << "\t\tUser recvd:\t" << stats.icmp.inputProcessed << std::endl;

			ss << "\tTx queued:\t\t" << stats.icmp.outputQueued << std::endl;
			ss << "\t\tWaiting:\t" << stats.icmp.getTxWaiting() << std::endl;

			ss << std::endl << "UDP stats: " << std::endl;

			ss << "\tReceived:\t\t" << stats.udp.inputReceived << std::endl;
			ss << "\t\tBad format:\t" << stats.udp.inputFormatError << std::endl;
			ss << "\t\tUser recvd:\t" << stats.udp.inputProcessed << std::endl;
			ss << "\t\tNo listener:\t" << stats.udp.inputNoPort << std::endl;

			ss << "\tTx queued:\t\t" << stats.udp.outputQueued << std::endl;
			ss << "\t\tWaiting:\t" << stats.udp.getTxWaiting() << std::endl;

			const char* str = ss.str().c_str();
			write(STDOUT_FILENO, str, strlen(str));

			Os::sleep(500);
		}
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
