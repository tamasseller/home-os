/*
 * NetStrings.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef NETERRORSTRINGS_H_
#define NETERRORSTRINGS_H_

struct NetErrorStrings {
	/*
	 * ICMP originated
	 */
	static constexpr const char* netUnreach = "Network unreachable (ICMP) message received";
	static constexpr const char* hostUnreach = "Host unreachable (ICMP) message received";
	static constexpr const char* protocolUnreach = "Protocol unreachable (ICMP) message received";
	static constexpr const char* portUnreach = "Port unreachable (ICMP) message received";
	static constexpr const char* mtuUnreach = "Fragmentation required (ICMP) message received";

	/*
	 * TCP
	 */
	static constexpr const char* alreadyConnected = "The socket is already connected";
	static constexpr const char* connectionReset = "Connection reset by peer";

	/*
	 * Generic and management
	 */
	static constexpr const char* allocError = "Too many buffer blocks requested";
	static constexpr const char* noRoute = "There is no known route leading to the specified network";
	static constexpr const char* unresolved = "The destination address could not be resolved (ARP)";
	static constexpr const char* genericTimeout = "Timeout occurred during communication";
	static constexpr const char* genericCancel = "Operation canceled";
};


#endif /* NETERRORSTRINGS_H_ */
