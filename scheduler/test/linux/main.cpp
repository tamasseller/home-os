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
	if(actualHandler)
		actualHandler();
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

    auto parentPid = getpid();

    if(fork() == 0) { // child process
    	while(1) {
    		usleep(1000);
    		kill(parentPid, SIGUSR2);
    	}
    }
    else
    	TestSuite<&testProgressReport, &testStartedReport, &registerIrqPtr>::runTests(1000);
}
