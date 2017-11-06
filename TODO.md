API
---

 - Implement IoRequest over IoChannel::Job.
 - Implement ReaderWriterLock (possibly with upgrade option).
 
Internals
---------

 - Handle mutexes in WaitableSet or disallow somehow.
 - Separte Blocker::remove into 'cancel' and 'timeout' calls.
 - Implement arbitrary base-class option (for add names to things and the like).
 - Implement mutex deadlock detection option.
 - Implement event tracing callback interface.
 - Fortify teardown process clearing syscall and tick handlers.

Backend
-------

 - Clean up linux context handling.
 - Find some way to keep track of high-res task running time.
 - Port to cortex-m3 (with real exclusive access instructions).
 - Port to cortex-m4f (with non-lazy float context saving).
 - Port to cortex-m7 (probably should be about the same as m4f).
