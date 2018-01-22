/*
 * DignosticCounters.h
 *
 *  Created on: 2018.01.08.
 *      Author: tooma
 */

#ifndef DIGNOSTICCOUNTERS_H_
#define DIGNOSTICCOUNTERS_H_

struct DiagnosticCounters {
	/**
	 * Protocol agnostic fields.
	 */
	template<class>
	struct Common {
		size_t inputReceived = 0;		//< Number of packets that entered the ingress processing pipeline
		size_t inputFormatError = 0;	//< Number of packets dropped due to format error (e.g. truncated or checksum)

		size_t outputQueued = 0;		//< Number of packets that entered the tranmission queue (for the protocol).
		size_t outputSent = 0;			//< Number of packets transmitted (for the protocol).

		inline size_t getTxWaiting() { return outputQueued - outputSent; }
	};

	/**
	 * Counters related to ARP request/response transmission and reception.
	 */
	struct Arp: Common<Arp> {
		size_t requestSent = 0;			//< Number of request packets transmitted.
		size_t replySent = 0;			//< Number of reply packets transmitted.

		size_t requestReceived = 0;		//< Number of request packets received.
		size_t replyReceived = 0;		//< Number of reply packets received.
	} arp;

	/**
	 * Counters related to various IP transmission and reception events.
	 */
	struct Ip: Common<Ip> {
		size_t inputReassemblyRequired = 0;	//< Number of fragmented packets received.
		size_t inputForwardingRequired = 0; //< Number of packets with a non local destination received.

		size_t outputRequest = 0;			//< Number of packet preparation attempts.
		size_t outputNoRoute = 0;			//< Number of packet preparation attempts failed due to lack of route.
		size_t outputArpFailed = 0;			//< Number of packet preparation attempts failed due to ARP failure.
	} ip;

	/**
	 * Counters related to various ICMP transmission and reception events.
	 */
	struct Icmp: Common<Icmp> {
		size_t inputProcessed = 0;			//< Number of packets that the user started processing.
		size_t pingRequests = 0;			//< Number of echo request packets received (processed automatically).
	} icmp;

	/**
	 * Counters related to various UDP transmission and reception events.
	 */
	struct Udp: Common<Udp> {
		size_t inputProcessed = 0;			//< Number of packets that the user started processing.
		size_t inputNoPort = 0;				//< Number of packets dropped due lack of a listener.
	} udp;

	/**
	 * Counters related to various UDP transmission and reception events.
	 */
	struct Tcp: Common<Tcp> {
		size_t inputProcessed = 0;			//< Number of packets that the user started processing.
		size_t inputConnectionDenied = 0;		//< Number of packets sent for indicating connection error.
		size_t inputConnectionAccepted = 0;		//< Number of packets sent for indicating connection error.
		size_t inputNoPort = 0;				//< Number of packets dropped due lack of a listener.
		size_t ackPackets = 0;				//< Number of packets sent for acknowledging received data.
		size_t rstPackets = 0;				//< Number of packets sent for indicating connection error.
	} tcp;

	template<class T>
	inline DiagnosticCounters(const T& storage):
		arp(storage),
		ip(storage),
		icmp(storage),
		udp(storage),
		tcp(storage) {}

	inline DiagnosticCounters() {}
};

template<bool enable>
struct DiagnosticCounterStorage;

template<>
struct DiagnosticCounterStorage<true>:
	DiagnosticCounters::Arp,
	DiagnosticCounters::Ip,
	DiagnosticCounters::Icmp,
	DiagnosticCounters::Udp,
	DiagnosticCounters::Tcp
{
	template<class T>
	inline void increment(size_t T::* field) {
		(this->*field)++;
	}
};

template<>
struct DiagnosticCounterStorage<false>: DiagnosticCounters {
	template<class T> inline void increment(size_t T::* field) {}
};

#endif /* DIGNOSTICCOUNTERS_H_ */
