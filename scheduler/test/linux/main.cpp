/*
 * main.cpp
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#include "common/TestSuite.h"

#include <stdint.h>
#include <malloc.h>
#include <ucontext.h>
#include <iostream>
#include <list>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

static sigset_t set;
void (* volatile actualHandler)();

void handlerTrampoline(int)
{
	void (*handler)() = actualHandler;

	if(handler)
		handler();
}

void testProgressReport(int)
{

}

void testStartedReport()
{

}


void registerIrqPtr(void (*handler)())
{
   	actualHandler = handler;
}

int main()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof (sa));
	sa.sa_handler = handlerTrampoline;
	sigfillset(&sa.sa_mask);
	sigaction(SIGUSR2, &sa, NULL);

	sigset_t set;
	sigemptyset (&set);
	sigaddset(&set, SIGUSR2);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

    auto parentPid = getpid();

    pid_t childPid = fork();
    if(childPid == 0) { // child process
    	do {
    		usleep(1000);
    	} while(kill(parentPid, SIGUSR2) == 0);
    } else {
    	return TestSuite<&testProgressReport, &testStartedReport, &registerIrqPtr>::runTests(1000);
    	kill(childPid, SIGTERM);
    }
}
