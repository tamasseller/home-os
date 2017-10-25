Goals
=====

The goals of the project are defined from two aspects:

 1. functional requirements, 
 2. and implementation constraints.

High-level functional requirements
----------------------------------

In addition to regular RTOS-es, the project aims to have:

 - Minimal platform dependence.
 - Optimal performance on cortex-m devices.
 - No global interrupt disable, on platforms where possible (where atomic access primitives are supported).
 - Support for fully static allocation of every object.
 - Ability to have configurable and extendable scheduling policies.
 - Configurable debug and security features, that need no (or minimal) changes to application code.

Differences from the mainstream RTOS-es:
 
 - No timeout locking of mutexes (timed waiting is supported for all non-mutex objects).
 - Supports waiting for multiple non-mutex synchronization objects.
 - No flag/event group primitive (multiple waiting can be used instead).
 - Non-mutex synchronization objects are customizable by application.
 
Implementation requirements
---------------------------

 - Truely object oriented structure.
 - Source language: C++11. (no ANSI c bullshit anymore, it's 2017)
 - Use real, virtual method call based, polymorphism where needed (no switch on enum values).
 - No macro magic, at all. Configuration is done through template parameters.
 - No _NUMBER_OF_SOMTHING_ arguments, application allocates everything statically.
 - Handling of task switching caused by high priority interrupt, whithout explicit end-of-interrupt-routine calls.
 
Overview
========

Operation
---------

Basically the job of the scheduler is to swap tasks around based on all sorts of constraints manifested in the
policies and synchronization objects, and maybe the priorities of the tasks. This means that all but one task 
is always waiting for something. There are two types of waiting: time-based and event-based. These two can be
mixed when the task is waiting for an event, but with a timeout. In that case it does both simultaneously.

The fact that a task is waiting for something needs to be kept track of by the scheduler, this is implemented
by putting the task (or an auxiliary object that can identify it) in the adequate container. So most of the 
basic activites of the scheduler can be described in terms of containers and their elements.

### Event-based waiting

If a tasks is waiting for an event, it said to be **blocked**, this is acheived by:

 - Trying to lock a mutex that is locked by another thread.
 - Waiting for a semaphore-like thing that blocks it.
 - Queuing for its turn on the CPU (commanded by the policy)
 
In each case the task is contained in blocker objects own container.
If a task waits for multiple semaphore type objects, then a many-to-many type of relation
needs to be represented. This is done by introducing several intermediary objects.

### Time-based waiting

If a task is waiting for a given instant of time, then it is also contained in a centralized sleep list.
The sleep list only ever contains tasks.

Objects
-------

The internal runtime objects that the system operates on are:

 - Mutex, a priority-inversion safe, recursive mutex that can be _locked_ and _unlocked_ whith **no timeout option**.
 - Waitable, base class for all non-mutex or semaphore-like objects. One or more can be waited for with or without 
   timeout (like unix _select_).
 - Policy: central storage for task that are ready to run. It implements the configured scheduling policy, for 
   selecting the next runnable task.

Interfaces
----------

The scheduler core has two main interfaces, one towards the application (the API) and one towards the hardware 
(called _profile_). Also there are several parameters that can be injected into the scheduler, like the policy
determining how to arbitrate ready-to-run tasks or debug and security mechanisms.

### Application interface

The application interface provides several classes and stand-alone functions:

 - Task
   - _start_: intializes the task object and puts it into the ready-to-run set, requires the stack and entry 
			  function to be specified, it may also require additional parameters depending on the policy.

 - Mutex class
   - _init_: initializes the mutex object (needed for the object to be reusable).
   - _lock_: obtains a lock or blocks until it can be got.
   - _unlock_: releases the mutex - depending on the policy - this can lead to immediate preemption (if a 
               higher priority task is waken).
   
 - Waitable-derived classes (ie non-mutex or semaphore-like snychronization objects)
   - _init_: initializes the object (needed for the object to be reusable).
   - _wait_: conditionally block the task (dictated by the actual subclass). There are two variants, one with a
			 timeout parameter and one without. The timeout variant can return true or false depending on the
			 cause of waking, if the waiting was timed-out, then it returns false. The other variant has no return
			 value.
   - _notify_: does whatever the subclass defines as the _notify_ operation. **This is the only operation that 
               can be issued from a high priority interrupt ('high' is platform dependent, basically one that can
               preempt the scheduler). For simple subclasses like semaphores this is what is known as _notify_/_send_
               in most contexts. In general it can cause to the concrete object to change internal state, which
               can lead to waking of blocked tasks.
   
  - Stand-alone
   - _start_: starts the scheduler, its parameters depend on the platform.
   - _yield_: voluntary relinquishment of CPU time, takes effect only there are other tasks ready to run, with
			  a static priority (if applicable for the policy) not greater then the current task.
   - _sleep_: avoid further execution for a predetermined time (passed as parameter).
   - _exit_: quit further execution immediately.

To enable testing the system in a simple, yet systematic manner the scheduler is able to restore the context in which
it was started in. When all the tasks exit the _start_ method of the scheduler simple returns (or at least it looks 
like that from the viewpoint of the application).

### Hardware interface (profile)

The component that implements platform dependent functionality has to expose an interface through which:

 - Atomic access to (at most) native word sized data can be made.
 - An asynchronous and mutliple synchronous mutually exclusive calls can be made (ie. syscall),
 - A timely interruption of execution can be acheived and handled by a handler routine.
 - The definition of a platform dependent execution context description,
 - A method to start executing of the first context, to exit the context cycling and return from where it was started.
 - Method to set up and swap the contexts.

Rationale
---------

There are several shortcomings of common microcontroller OS-es both regarding functionality and implemenatation.

### Decoupling

As microcontrollers get more and more beefy, people tend to implement software functions on them, 
that where never meant to be run on anything other than a real computer. Several examples are:

 - Full functionality TCP/IP networking and bridging with esoteric communication channels.
 - Real, browser-facing web services, with all sorts of realtime communication (like websockets).
 - Graphical interfaces with sophisticated visuals and controls (relatively high-res, true-color touchscreens).
 - Script engines and VMs (python, js, java, etc...)
 - Large-scale (compared to MCU parameters) storage involving sophisticated file-systems.
 
These are not the old-school, quickly hacked together, few hundred lines worth of assembly type software, these
software modules can only be developed fully and efficiently if they are modular and reusable, and thus testable.
Common RTOS-es do not provide good enough tools and interfaces to acheive this.

A good example is the event/flag group (with a different name on different RTOS-es) that is used to work around
the lack of ability to wait for any of a set of semaphore-like object in a _select_ like manner.
This synchronization primitive needs the application to statically assign a bit in a bitfield to all channels, 
that can be waited for, which is obviously can be done easily in an automatic way.

It is a better alternative to provide capability to **wait for a set of arbitrary events** simultaneously.
This way a component that needs to implement internal synchronization can have its own semaphores that are 
virtually invisible for the application that uses the module.

### Realtimeness

Microcontrolelr OS-es that claim to be hard realtime are either extremely primitive or not really that hard
realtime at all, or at least have very poor upper limits on delays. And that is like so, because a usable task
scheduling scheme can not be managed in constant time. There are several collections of objects that are required
to be kept sorted, which is simply impossible to with a time-complexity which is constant in the number of 
elements on a real-life processor.

So instead of trying to create the impossible it is much wiser, to let the hardware assist in doing the hard-realtime
computations using adequatily prioritized interrupt routines. It is a better goal for a scheduler to delay these as
little as possible by **leaving the interrupt handlers enabled all the time** (when the hardware enables it and 
minimize the number of instructions in critical sections when it does not).

This way the scheduling can be done with a bit more relaxed timing constraints, without hurting the real realtime part.
This of course implies that, if the high priority interrupts need to communicate whith the tasks, then some kind of
lock-free magic is required for that purpose. Luckily enough, CPUs of modern 32-bit microcontrollers provide hardware
support for atomic access primitives. In addition, on targets that do not provide these facilites, an atomic access 
implementation can fall back to global interrupt masking on extremely short sections, thus minimizing the effect of
the scheduler on the delay of interrupt servicing.

### Implementation

Although task schedulers are object oriented by their very nature, due to the common misbelief that system-level
software can only be written in plain C, almost all RTOS-es are written using some ad-hoc object representations.
This is not so much of a problem for non-polymorphic classes, but when actual runtime polymorphism is needed, then
people need to invent there own _vtables_. Same thing as the polymorphic classes in the linux  kernel, all done 
by hand (the _foo_operation_ structs, consisting of only pointers to functions that take the pointer to the object
as the first argument seem to be pretty much the same as what a C++ compiler would have  generated as vtable for a
polymorphic class).

Using C also means that the only way to implement compile time configuration is using the preprocessor, that does not
actually understand the language and only substitutes string blindly. That has several important imlications:
 
 - Numeric parameters (and function pointers also) are injected mostly unchecked.
 - Configurable segments of code must be guarded by _ifdef_ blocks.
 - No actual compile-time evaluation other than simple arithmetics.

Much better work can be done (regarding readability and maintainability) by using the features of a C++ compiler, 
namely **automatic handling of polymorphism and the generic programming enabled by template classes**.
