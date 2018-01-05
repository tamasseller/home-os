/*
 * main.cpp
 *
 *  Created on: 2018.01.04.
 *      Author: tooma
 */

#include "System.h"

#include <iostream>

Net::Buffers packetBuffers;

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
