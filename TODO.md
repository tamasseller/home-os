API
---

 - Create separate configuration object and make Scheduler and Network use that single config object instead of the options themselfs.
 - Create central header file that insists on including some user provided sysconfig header, and expects a config object in it that is then used to define Scheduler and Network.
 - Separate implementation of stuff that are not to be inlined (core things behind syscall and async) from the rest.
 - Move implementations of internally used method to separate .impl.h files.
 - Make a single cpp file per component (net, scheduler) that includes all the .impl.h files and contains explicit specializations.
 
Internals
---------

 - Get rid of pluggable scheduling policy stuff.
 - Implement fair scheduling policy (needs hires timing).
 - Make highest prority task inhibit async messages.
 - Add detection of elevated (syscall/async) context method to arch.
 - Use isElevated method to handle aborting properly (also reenable processing if highest prio).
 - Refactor the select interface into an epoll like one and also handle return value and index of the waker properly (needed for IoRequest).
 - TCP: implement in-task sending (with wait for window).
 - TCP: add retranmission timeout for window waiting (also implements zero window probing?).
 - TCP: implement connection close signaling.
 - TCP: implement waiting for connection closing (with retransmission timeout).
 - NET: move implementation of inet checksum to arch.
 - NET: add support for checksum offloading.
 - NET: add support for receiving broadcast packets.
 - NET: move well-known constants (ip protocol numbers, ethertypes, fixed packet sizes) to some central header.
 - NET: add ip reassembly support.
 - Reform IoJob/IoRequest (CRTP IoJobBase maybe?) to enable more flexibility in controlling what is done in async context and what in task.
 - Add random generator option.
 - NET: Use random generator for ip id fields and tcp initial seq nums.
 - NET: implement receive window size handling (check if windows shrinking is applicable, if it is then use the amount of globally available memory to maximize performance)

Backend
-------

 - Optimize cortex-m syscall (seemingly no need for four param syscall, use {r0-4} instead of {r12, r0-4})
 - Find some way to keep track of high-res task running time.
 - Port to cortex-m3 and up (with real exclusive access instructions).
