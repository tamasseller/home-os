/*
 * NetErrorStrings.h
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
	static constexpr const char* unknownError = "Bullshit";
	static constexpr const char* netUnreachError = "Network unreachable (ICMP) message received";
	static constexpr const char* hostUnreachError = "Host unreachable (ICMP) message received";
	static constexpr const char* protocolUnreachError = "Protocol unreachable (ICMP) message received";
	static constexpr const char* portUnreachError = "Port unreachable (ICMP) message received";
	static constexpr const char* mtuUnreachError = "Fragmentation required (ICMP) message received";

	/*
	 * TCP
	 */
	static constexpr const char* alreadyConnectedError = "The socket is already connected";
	static constexpr const char* connectionResetError = "Connection reset by peer";
	static constexpr const char* outOfSegmentsError = "No TCP segment descriptor available";

	/*
	 * Generic and management
	 */
	static constexpr const char* alreadyUsedError = "The asynchronous control block is still being used.";
	static constexpr const char* noRouteError = "There is no known route leading to the specified network.";
	static constexpr const char* unresolvedError = "The destination address could not be resolved. (ARP)";
	static constexpr const char* genericTimeoutError = "Timeout occured during communication";
};


#endif /* NETERRORSTRINGS_H_ */
