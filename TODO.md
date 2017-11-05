API
---

 - Implement ReaderWriterLock (possibly with upgrade option).
 - Implement std::future+std::promise like, non-shared blocker.
 - Implement driver-oriented io queue and job objects for 
   interrupt-driven processing, that runs in scheduler context
   to cheaply implement mutual exclusion and also handles:

   - Enabling and disabling the related interrupt (to acheive full
     mutual exclusion) the exact details of which are provided by
     the user (via subclassing).
   - Synchronization of enabling/disabling with queue operations.
   - Adding an - optionally prioritized - work item from a task
     or another interrupt which may trigger enabling the process.
   - Canceling a work item (ie. on timeout) and disabling the 
     interrupt, if needed.
   - Popping a work item (used from the process interrupt)
   - Two kinds of work items:
     - future-promise based for tasks,
     - arbitrary callback based for interrupts.
 
Internals
---------

 - Implement arbitrary base-class option (for add names to things and the like).
 - Implement mutex deadlock detection option.
 - Implement event tracing callback interface.

Backend
-------

 - Find some way to keep track of high-res task running time.
 - Port to cortex-m3 (with real exclusive access instructions).
 - Port to cortex-m4f (with non-lazy float context saving).
 - Port to cortex-m7 (probably should be about the same as m4f).
