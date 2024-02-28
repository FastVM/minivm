
EXTRA_SRCS = vendor/c11threads/threads_posix.c
CFLAGS := -I vendor/c11threads $(CFLAGS) 

include makefile
