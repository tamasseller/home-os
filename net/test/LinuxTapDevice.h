/*
 * LinuxTapDevice.h
 *
 *  Created on: 2017.11.13.
 *      Author: tooma
 */

#ifndef LINUXTAPDEVICE_H_
#define LINUXTAPDEVICE_H_

#include <fcntl.h>
#include <string.h>

#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <stdio.h>  // XXX

//#define PRINT_PACKETS 1

template<class Net, uint16_t blockMaxPayload>
class LinuxTapDevice {
	friend Net;
    static constexpr size_t arpCacheEntries = 8;
    static constexpr auto ethernetAddress = AddressEthernet::make(0xee, 0xee, 0xee, 0xee, 0xee, 0);

    static constexpr const char *interfaceName = "tap0";
    static constexpr auto nBlocks = (1500 + blockMaxPayload - 1) / blockMaxPayload;
	char* rxTable[nBlocks + 1];
	int fd, rxWriteIdx, rxReadIdx;

	inline char* receiveSome(const uint8_t *&p, uint16_t &length, uint16_t &run)
	{
		char* ret = rxTable[rxReadIdx];
		rxTable[rxReadIdx] = nullptr;
		rxReadIdx = (rxReadIdx + 1) % (nBlocks + 1);

		run = (length < blockMaxPayload) ? length : blockMaxPayload;

		memcpy(ret, p, run);
		p += run;

		length = static_cast<uint16_t>(length - run);

		return ret;
	}

	static void sig_io(int sig)
	{
		typename Net::PacketAssembler assembler;
		uint16_t nBlocksConsumed = 1, length, run;
		static constexpr auto buffsize = blockMaxPayload*nBlocks;
		static char buffer[buffsize];
		ssize_t nread;

		auto self = Net::template getEthernetInterface<LinuxTapDevice>();

		while((nread = read(self->fd, buffer, buffsize)) > 0) {
			if(!((self->rxWriteIdx - self->rxReadIdx + (nBlocks + 1)) % (nBlocks + 1)))
				return;

#ifdef PRINT_PACKETS
			for(ssize_t i = 0; i < nread; i++){
				if((i & 15) == 0) printf("RX ");
				printf("%02X ", (unsigned int)(unsigned char)buffer[i]);
				if((i & 15) == 15) printf("\n");
				else if((i & 7) == 7) printf(" ");
			}
			printf("\n\n");
#endif

			const uint8_t *p = (const uint8_t*)buffer;
			uint16_t length = (uint16_t)nread;

			char* chunk = self->receiveSome(p, length, run);
			assembler.initByFinalInlineData(chunk, run);
			uint16_t nBlocksConsumed = 1;

			while(length) {
				if(!((self->rxWriteIdx - self->rxReadIdx + (nBlocks + 1)) % (nBlocks + 1))) {
					assembler.done();
					assembler.template dispose<Net::Pool::Quota::Rx>();
					self->requestRxBuffers(nBlocksConsumed);
					return;
				}

				char* chunk = self->receiveSome(p, length, run);
				assembler.addBlockByFinalInlineData(chunk, run);
				nBlocksConsumed++;
			}

			assembler.done();

			auto self = Net::template getEthernetInterface<LinuxTapDevice>();
			self->requestRxBuffers(nBlocksConsumed);

			if(!buffer[13]) /* 13: second byte of ethertype */
				self->ipPacketReceived(assembler);
			else
				self->arpPacketReceived(assembler);
		}
	}

	inline void init() {
		rxWriteIdx = rxReadIdx = 0;
		Net::template getEthernetInterface<LinuxTapDevice>()->requestRxBuffers(nBlocks);

		Net::Os::assert((fd = open("/dev/net/tun", O_RDWR)) >= 0, "Failed to open '/dev/net/tun'");

		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
		strcpy(ifr.ifr_name, interfaceName);

		Net::Os::assert(ioctl(fd, TUNSETIFF, (void*)&ifr) >= 0, "Failed ioctl(TUNSETIFF)");

		ioctl(fd, TUNSETNOCSUM, 1);
		fcntl(fd, F_SETFL, O_NONBLOCK | O_ASYNC);

		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = &LinuxTapDevice::sig_io;
		sigaction(SIGIO, &sa, NULL);
	}

	inline void disableTxIrq() {
	}

	inline void enableTxIrq() {
		sigset_t mask, old;
		sigfillset(&mask);
		sigprocmask(SIG_SETMASK, &mask, &old);

	    auto x = Net::template getEthernetInterface<LinuxTapDevice>();
	    while(auto* p = x->getCurrentTxPacket()) {
	        typename Net::PacketStream packet;
	        packet.init(*p);

#ifdef PRINT_PACKETS
			for(ssize_t i = 0; !packet.atEop(); i++) {
				if((i & 15) == 0) printf("TX ");
				uint8_t b;
				packet.read8(b);
				printf("%02X ", (unsigned int)b);
				if((i & 15) == 15) printf("\n");
				else if((i & 7) == 7) printf(" ");
			}

			printf("\n\n");
#endif

			int n = 0;
			struct iovec iov[nBlocks];

			packet.init(*p);
			while(!packet.atEop()) {
				typename Net::Chunk chunk = packet.getChunk();
				iov[n].iov_base = chunk.start;
				iov[n].iov_len = chunk.length();
				packet.advance(static_cast<uint16_t>(chunk.length()));
				n++;
				Net::Os::assert(n < nBlocks, "DAFUQ?");
			}

			Net::Os::assert(writev(fd, iov, n) >= 0, "Could not write tap device");

	        x->packetTransmitted();
	    }

	    sigprocmask(SIG_SETMASK, &old, nullptr);
	}

	inline bool takeRxBuffer(char* data) {
		sigset_t intmask;
		sigemptyset(&intmask);
		sigaddset(&intmask, SIGINT);
		sigprocmask(SIG_BLOCK, &intmask, NULL);

		rxTable[rxWriteIdx] = data;
		rxWriteIdx = (rxWriteIdx + 1) % (nBlocks + 1);

		sigprocmask(SIG_UNBLOCK, &intmask, NULL);
		return true;
	}

public:
	inline void setTapPeerAddress(const char* addrStr) {
		struct sockaddr_in sai;
		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ);

		memset(&sai, 0, sizeof(struct sockaddr));
		sai.sin_family = AF_INET;
		sai.sin_port = 0;
		sai.sin_addr.s_addr = inet_addr(addrStr);

		memcpy((char*)&ifr.ifr_addr, (const char*)&sai, sizeof(struct sockaddr));
		ioctl(sockfd, SIOCSIFADDR, &ifr);

		ioctl(sockfd, SIOCGIFFLAGS, &ifr);
		ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
		ioctl(sockfd, SIOCSIFFLAGS, &ifr);

		close(sockfd);
	}
};

template<class Net, uint16_t blockMaxPayload>
constexpr AddressEthernet LinuxTapDevice<Net, blockMaxPayload>::ethernetAddress;


#endif /* LINUXTAPDEVICE_H_ */
