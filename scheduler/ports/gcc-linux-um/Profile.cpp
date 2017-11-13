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

#include "Profile.h"

namespace home {

void (*ProfileLinuxUm::tickHandler)();
void (* volatile ProfileLinuxUm::asyncCallHandler)();
void* (* volatile ProfileLinuxUm::syncCallMapper)(uintptr_t);

volatile uint32_t ProfileLinuxUm::tick = 0;

ProfileLinuxUm::Task* volatile ProfileLinuxUm::currentTask;
ProfileLinuxUm::Task* volatile ProfileLinuxUm::oldTask;
const char* ProfileLinuxUm::returnValue;
ucontext_t ProfileLinuxUm::final;
ucontext_t ProfileLinuxUm::idle;
uintptr_t ProfileLinuxUm::idleStack[1024];
bool ProfileLinuxUm::suspend;
bool ProfileLinuxUm::suspended;

void ProfileLinuxUm::enableSignals() {
	sigset_t set;
	sigemptyset (&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}

void ProfileLinuxUm::disableSignals() {
	sigset_t set;
	sigemptyset (&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, NULL);
}

void ProfileLinuxUm::init(uintptr_t tickUs)
{
	disableSignals();

    getcontext(&idle);
    idle.uc_link = 0;
    idle.uc_stack.ss_sp = idleStack;
    idle.uc_stack.ss_size = sizeof(idleStack);
    idle.uc_stack.ss_flags = 0;
    sigemptyset(&idle.uc_sigmask);
    makecontext(&idle, (void (*)())idler, 0);

    struct sigaction sa;

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &sigAlrmHandler;
    sigfillset(&sa.sa_mask);
    sigaction (SIGALRM, &sa, NULL);

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &ProfileLinuxUm::sigUsr1Handler;
    sigfillset(&sa.sa_mask);
    sigaction (SIGUSR1, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = tickUs / 1000000;
    timer.it_value.tv_usec = tickUs % 1000000;
    timer.it_interval.tv_sec = tickUs / 1000000;
    timer.it_interval.tv_usec = tickUs % 1000000;
    setitimer (ITIMER_REAL, &timer, NULL);

    tick = 0;
}


void ProfileLinuxUm::sigAlrmHandler(int) {
	tick++;
	tickHandler();
}

void ProfileLinuxUm::sigUsr1Handler(int sig)
{
	Task* task = realCurrentTask();

	if(task->syscall) {
		void (* call)(Task*) = task->syscall;
		task->syscall = nullptr;
		call(task);
	}

	if(asyncCallHandler) {
		void (* handler)() = asyncCallHandler;
		asyncCallHandler = nullptr;
		handler();
	}

	if(oldTask) {
		if(suspended) {
			suspended = false;
			oldTask = nullptr;
			swapcontext(&idle, &currentTask->savedContext);
		} else {
			Task* old = oldTask;
			oldTask = nullptr;
			swapcontext(&old->savedContext, &currentTask->savedContext);
		}
	} else if(suspend) {
		suspend = false;
		suspended = true;
		swapcontext(&currentTask->savedContext, &idle);
	}
}

void ProfileLinuxUm::dispatchSyscall(Task* self) {
	uintptr_t *args = self->syscallArgs.args;
	void* functPtr = syncCallMapper(args[0]);

	switch(self->syscallArgs.nArgs) {
	case 0:
		self->returnValue = ((uintptr_t (*)())functPtr)();
		break;
	case 1:
		self->returnValue = ((uintptr_t (*)(uintptr_t))functPtr)(args[1]);
		break;
	case 2:
		self->returnValue = ((uintptr_t (*)(uintptr_t, uintptr_t))functPtr)(args[1], args[2]);
		break;
	case 3:
		self->returnValue = ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t))functPtr)(args[1], args[2], args[3]);
		break;
	case 4:
		self->returnValue = ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))functPtr)(args[1], args[2], args[3], args[4]);
		break;
	}
}

void ProfileLinuxUm::idler()
{
	while(true) {
		pause();
	}
}

const char* ProfileLinuxUm::startFirst(Task* task)
{
	currentTask = task;

	swapcontext(&final, &task->savedContext);

	struct itimerval timer;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer (ITIMER_REAL, &timer, NULL);

	usleep(1000);

	//enableSignals();
	return returnValue;
}

void ProfileLinuxUm::finishLast(const char* errorMessage) {
	ProfileLinuxUm::returnValue = errorMessage;
	currentTask = nullptr;
	setcontext(&final);
}

void ProfileLinuxUm::initialize(Task* task, void (*entry)(void*), void (*exit)(), void* stack, uintptr_t stackSize, void* arg)
{
	task->syscall = nullptr;

	bzero(&task->exitContext, sizeof(task->exitContext));
    getcontext(&task->exitContext);
    task->exitContext.uc_link = 0;
    task->exitContext.uc_stack.ss_sp = task->exitStack;
    task->exitContext.uc_stack.ss_size = sizeof(Task::exitStack);
    task->exitContext.uc_stack.ss_flags = 0;
    sigemptyset(&task->exitContext.uc_sigmask);
    makecontext(&task->exitContext, exit, 0);

    bzero(&task->savedContext, sizeof(task->savedContext));
    getcontext(&task->savedContext);
    task->savedContext.uc_link = &task->exitContext;
    task->savedContext.uc_stack.ss_sp = stack;
    task->savedContext.uc_stack.ss_size = stackSize;
    task->savedContext.uc_stack.ss_flags = 0;
    sigemptyset(&task->savedContext.uc_sigmask);
    makecontext(&task->savedContext, (void (*)())entry, 1, arg);
}

}
