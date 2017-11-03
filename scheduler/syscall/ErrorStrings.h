/*
 * ErrorStrings.h
 *
 *  Created on: 2017.11.03.
 *      Author: tooma
 */

#ifndef ERRORSTRINGS_H_
#define ERRORSTRINGS_H_

template<class... Args>
struct Scheduler<Args...>::ErrorStrings {
	static constexpr const char* policyBlockerUsage = "Only priority change can be handled through the Blocker interface of the Policy wrapper";
	static constexpr const char* policyNonTask = "Only tasks can be handled by the policy container";
};

#endif /* ERRORSTRINGS_H_ */
