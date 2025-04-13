CC=gcc
CFLAGS=
FMT=indent

mysh: shell.c interpreter.c shellmemory.c scheduler.c
	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c scheduler.c
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o scheduler.o

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h scheduler.c scheduler.h scheduler.c scheduler.h test.c
	$(FMT) $?

clean: 
	$(RM) mysh; $(RM) *.o; $(RM) *~

test: test.c
	$(CC) $(CFLAGS) -o test test.c

# Run the test program
run_test: test
	./test
