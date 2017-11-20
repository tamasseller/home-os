/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

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

    std::set_terminate([](){
        std::cout << "Unhandled exception\n";
        std::abort();
    });

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
