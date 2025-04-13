CC=gcc
#CFLAGS=-g -O0 #-DNDEBUG
CFLAGS=-DNDEBUG
FMT=indent

# Default values if not provided
framesize ?= 6  # Total lines in frame store
varmemsize ?= 10  # Total lines in variable store

mysh: shell.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -D FRAME_STORE_SIZE=$(framesize) -D VAR_MEM_SIZE=$(varmemsize) -c shell.c interpreter.c shellmemory.c pcb.c queue.c schedule_policy.c 
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o pcb.o queue.o schedule_policy.o 

clean: 
	$(RM) mysh; $(RM) *.o; $(RM) *~


test: test.c
	$(CC) $(CFLAGS) -o test test.c


utest: utest.c shellmemory.c pcb.c
	$(CC) $(CFLAGS) -D FRAME_STORE_SIZE=$(framesize) -D VAR_MEM_SIZE=$(varmemsize) -o utest utest.c shellmemory.c pcb.c



style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h pcb.c pcb.h queue.c queue.h schedule_policy.c schedule_policy.h utest.c test.c
	$(FMT) $?
