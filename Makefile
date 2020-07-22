CC = clang

CFLAGS = -O3 -DNDEBUG
DBGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g

task2aig: task2aig.c aiger/aiger.c aiger/aiger.h
	$(CC) $(CFLAGS) -lm -o task2aig aiger/aiger.c task2aig.c

aigprod: aigprod.c aiger/aiger.c aiger/aiger.h
	$(CC) $(DBGFLAGS) -o aigprod aiger/aiger.c aigprod.c

.PHONY: clean all

all: task2aig aigprod

clean:
	rm -f task2aig
	rm -f aigprod
