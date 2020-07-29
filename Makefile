CC = clang

CFLAGS = -O3 -DNDEBUG
DBGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g

task2aig: task2aig.c aiger/aiger.c aiger/aiger.h
	$(CC) $(CFLAGS) -lm -o task2aig aiger/aiger.c task2aig.c

aigprod: aigprod.c aiger/aiger.c aiger/aiger.h
	$(CC) $(CFLAGS) -o aigprod aiger/aiger.c aigprod.c

aiger/aiger_wrap.c: aiger/aiger.i
	swig -python aiger/aiger.i

.PHONY: clean all

all: task2aig aigprod aiger/aiger_wrap.c

clean:
	rm -f task2aig
	rm -f aigprod
