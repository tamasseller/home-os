/*
 * Profile.cpp
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#include "Profile.h"

ProfileLinuxUm::Task* volatile ProfileLinuxUm::Task::currentTask;
ProfileLinuxUm::Task* volatile ProfileLinuxUm::Task::oldTask;
bool ProfileLinuxUm::Task::suspend;
bool ProfileLinuxUm::Task::suspended;

ucontext_t ProfileLinuxUm::Task::final;
ucontext_t ProfileLinuxUm::Task::idle;
uintptr_t ProfileLinuxUm::Task::idleStack[1024];


volatile uint32_t ProfileLinuxUm::Timer::tick = 0;

void (*ProfileLinuxUm::Timer::tickHandler)();
void (* volatile ProfileLinuxUm::CallGate::asyncCallHandler)();
void* (* volatile ProfileLinuxUm::CallGate::syncCallMapper)(void*);

void ProfileLinuxUm::Timer::sigAlrmHandler(int) {
	tick++;
	tickHandler();
}

void ProfileLinuxUm::sigUsr1Handler(int sig)
{
	Task* realCurrentTask = Task::realCurrentTask();

	if(realCurrentTask->syscall) {
		void (* call)(Task*) = realCurrentTask->syscall;
		realCurrentTask->syscall = nullptr;
		call(realCurrentTask);
	}

	if(CallGate::asyncCallHandler) {
		void (* handler)() = CallGate::asyncCallHandler;
		CallGate::asyncCallHandler = nullptr;
		handler();
	}

	if(Task::oldTask) {
		if(Task::suspended) {
			Task::suspended = false;
			Task::oldTask = nullptr;
			swapcontext(&Task::idle, &Task::currentTask->savedContext);
		} else {
			Task* old = Task::oldTask;
			Task::oldTask = nullptr;
			swapcontext(&old->savedContext, &Task::currentTask->savedContext);
		}
	} else if(Task::suspend) {
		Task::suspend = false;
		Task::suspended = true;
		swapcontext(&Task::currentTask->savedContext, &Task::idle);
	}
}

void ProfileLinuxUm::init(int tickUs)
{
    sigset_t set;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);

    getcontext(&Task::idle);
    Task::idle.uc_link = 0;
    Task::idle.uc_stack.ss_sp = Task::idleStack;
    Task::idle.uc_stack.ss_size = sizeof(Task::idleStack);
    Task::idle.uc_stack.ss_flags = 0;
    makecontext(&Task::idle, (void (*)())Task::idler, 0);

    struct sigaction sa;

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &Timer::sigAlrmHandler;
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

    sigdelset(&set, SIGALRM);
    sigdelset(&set, SIGUSR1);
    sigdelset(&set, SIGINT);
    sigprocmask(SIG_SETMASK, &set, NULL);
}

void ProfileLinuxUm::Task::startFirst() {
	currentTask = this;

	sigset_t sig;
	sigemptyset (&sig);
	sigaddset(&sig, SIGUSR1);
	sigaddset(&sig, SIGALRM);
	sigprocmask(SIG_BLOCK, &sig, NULL);

	swapcontext(&final, &savedContext);

	struct itimerval timer;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer (ITIMER_REAL, &timer, NULL);

	usleep(1000);

	sigprocmask(SIG_UNBLOCK, &sig, NULL);

}

void ProfileLinuxUm::Task::finishLast() {
	setcontext(&final);
}

void ProfileLinuxUm::Task::initialize(void (*entry)(void*), void (*exit)(), void* stack, uint32_t stackSize, void* arg)
{
    getcontext(&exitContext);
    exitContext.uc_link = 0;
    exitContext.uc_stack.ss_sp = exitStack;
    exitContext.uc_stack.ss_size = sizeof(exitStack);
    exitContext.uc_stack.ss_flags = 0;
    makecontext(&exitContext, (void (*)())exit, 0);

    getcontext(&savedContext);
    savedContext.uc_link = &exitContext;
    savedContext.uc_stack.ss_sp = stack;
    savedContext.uc_stack.ss_size = stackSize;
    savedContext.uc_stack.ss_flags = 0;
    makecontext(&savedContext, (void (*)())entry, 1, arg);
}
