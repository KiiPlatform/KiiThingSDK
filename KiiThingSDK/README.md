Kii Thing SDK for C language is designed to used by several Things connected to internet.

## Required libraries
- jansson 2.7.0
    http://www.digip.org/jansson/

## Dependency
- jansson
    http://www.digip.org/jansson/
- libcurl
    http://curl.haxx.se/libcurl/
- POSIX apis

## Thread safety
To make a thread safe program, you need to take care following points.
- kii_global_init(void)/ kii_global_cleanup(void) is not thread safe.<br>
  You must not call it when any other thread in the program (i.e. a thread sharing the same memory) is running.<br>
  This doesn't just mean no other thread that is using kii sdk. Because kii_global_init(void)/ kii_global_cleanup(void) calls functions of other libraries that are similarly thread unsafe, it could conflict with any other thread that uses these other libraries.

- Don't share instance which is passed to SDK apis as non-const argument among the threads.
