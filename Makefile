CC = clang

CFLAGS = -O3 -DNDEBUG
DBGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g

task2aig: $(SRCS) $(HDRS) task2aig.c aiger/aiger.c aiger/aiger.h
	$(CC) $(CFLAGS) -lm -o task2aig aiger/aiger.c task2aig.c

.PHONY: clean

clean:
	rm -f task2aig
