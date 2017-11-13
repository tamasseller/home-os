API
---

 - Implement ReaderWriterLock (possibly with upgrade option).
 
Internals
---------

 - Implement arbitrary base-class option (for add names to things and the like).
 - Implement event tracing callback interface.
 - Implement fair scheduling policy (needs hires timing)

Backend
-------

 - Clean up linux context handling.
 - Find some way to keep track of high-res task running time.
 - Port to cortex-m3 (with real exclusive access instructions).
 - Port to cortex-m4f (with non-lazy float context saving).
 - Port to cortex-m7 (probably should be about the same as m4f).

Documentation
-------------

 - Overview:
   - Preemption levels (task, syscall, user interrupt)
   - Event list based communication
   - Generic blocker things
   - Multi wait scenario
   - Continuation
 - Doxy comments
   - Blocker.h
   - SemaphoreLikeBlocker.h
   - AsyncBlocker.h
   - WaitableSet.h
   - Sleeping.h
   - SharedBlocker.h
   - Policy.h
   - Blockable.h
   - IoRequest.h
   - IoChannel.h
   - Task.h
   - Mutex.h
     - Explanation Blocker private base (inhibit select)
   - BinarySemaphore.h
   - CountingSemaphore.h
   - Preemption.h
   - Event.h
   - EventList.h
   - SharedAtomicList.h
   - Atomic.h
   - Timeout.h
   - RealtimePolicy.h
   - RoundRobinPrioPolicy.h
   - RoundRobinPolicy.h
   - Syscall.h
   - ObjectRegistry.h
   - LimitedCTree.h
   - Helpers.h
   - ErrorStrings.h
 - Tests:
   - MutexDeadlock.cpp
   - SemaphoreFromIrq.cpp
   - TaskStart.cpp
   - SemaphorePrio.cpp
   - IoChannelTimeout.cpp
   - Yield.cpp
   - PolicyPrio.cpp
   - TestLimitedCTreeHostOnly.cpp
   - SemaphorePassaround.cpp
   - Error.cpp
   - MutexMany.cpp
   - SemaphoreCounting.cpp
   - SharedAtomicListSimple.cpp
   - MutexVsSelect.cpp
   - MutexSingle.cpp
   - SemaphoreTimeout.cpp
   - Sleep.cpp
   - IoChannel.cpp
   - MutexSingleBusy.cpp
   - MutexPriorityInversion.cpp
   - Abort.cpp
   - Select.cpp
   - Timeout.cpp
   - SemaphoreLock.cpp
   - LimitedCTree.cpp
   - SharedAtomicListReissue.cpp
   - Policy.cpp
