API
---

 - Move sleep/yield/etc into Task from Scheduler.
 - Create forced scheduler exit for error handling.
 - Ensure correct finalization for forced exit.
 - Implement ReaderWriterLock (possibly with upgrade option).
 
Internals
---------

 - Implement arbitrary base-class option (for add names to things and the like).
 - Implement checked syscall option.
 - Implement object registry option.
 - Implement mutex deadlock detection option.
 - Implement event tracing callback interface.
 - Introduce helper for priority setting.
 - Introduce proper method for comparing priorities in policy instead of < operator.

Backend
-------

 - Find some way to keep track of high-res task running time.
 - Port to cortex-m3 (with real exclusive access instructions).
 - Port to cortex-m4f (with non-lazy float context saving).
 - Port to cortex-m7 (probably should be about the same as m4f).
