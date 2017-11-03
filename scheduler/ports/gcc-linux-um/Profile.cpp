/*
 * Profile.cpp
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#include "Profile.h"

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

void ProfileLinuxUm::init(uintptr_t tickUs)
{
    sigset_t set;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);

    getcontext(&idle);
    idle.uc_link = 0;
    idle.uc_stack.ss_sp = idleStack;
    idle.uc_stack.ss_size = sizeof(idleStack);
    idle.uc_stack.ss_flags = 0;
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

    sigdelset(&set, SIGALRM);
    sigdelset(&set, SIGUSR1);
    sigdelset(&set, SIGINT);
    sigprocmask(SIG_SETMASK, &set, NULL);
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
	sigset_t mask, oldMask;
	sigfillset(&mask);

	while(1) {
		sigprocmask(SIG_UNBLOCK, &mask, &oldMask);
		pause();
		sigprocmask(SIG_SETMASK, &oldMask, 0);
	}
}

const char* ProfileLinuxUm::startFirst(Task* task)
{
	currentTask = task;

	sigset_t sig;
	sigemptyset (&sig);
	sigaddset(&sig, SIGUSR1);
	sigaddset(&sig, SIGALRM);
	sigprocmask(SIG_BLOCK, &sig, NULL);

	swapcontext(&final, &task->savedContext);

	struct itimerval timer;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer (ITIMER_REAL, &timer, NULL);

	usleep(1000);

	sigprocmask(SIG_UNBLOCK, &sig, NULL);
	return returnValue;
}

void ProfileLinuxUm::finishLast(const char* errorMessage) {
	ProfileLinuxUm::returnValue = errorMessage;
	setcontext(&final);
}

void ProfileLinuxUm::initialize(Task* task, void (*entry)(void*), void (*exit)(), void* stack, uintptr_t stackSize, void* arg)
{
	bzero(&task->exitContext, sizeof(task->exitContext));
    getcontext(&task->exitContext);
    task->exitContext.uc_link = 0;
    task->exitContext.uc_stack.ss_sp = task->exitStack;
    task->exitContext.uc_stack.ss_size = sizeof(Task::exitStack);
    task->exitContext.uc_stack.ss_flags = 0;
    makecontext(&task->exitContext, exit, 0);

    bzero(&task->savedContext, sizeof(task->savedContext));
    getcontext(&task->savedContext);
    task->savedContext.uc_link = &task->exitContext;
    task->savedContext.uc_stack.ss_sp = stack;
    task->savedContext.uc_stack.ss_size = stackSize;
    task->savedContext.uc_stack.ss_flags = 0;
    makecontext(&task->savedContext, (void (*)())entry, 1, arg);
}
