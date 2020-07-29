CC = clang

CFLAGS = -O3 -DNDEBUG
DBGFLAGS = -fsanitize=address -fno-omit-frame-pointer -g

task2aig: $(SRCS) $(HDRS) task2aig.c aiger/aiger.c aiger/aiger.h
	$(CC) $(CFLAGS) -lm -o task2aig aiger/aiger.c task2aig.c

aiger/_aiger_wrap.so: aiger/aiger_wrap.c aiger/aiger.c
	$(CC) $(CFLAGS) \
		-shared -Wl,-undefined,dynamic_lookup \
		-fPIC -o aiger/_aiger_wrap.so \
		-I/usr/include/python3.0 \
		-lpython3.0 \
	        aiger/aiger_wrap.c \
		aiger/aiger.c

aiger/aiger_wrap.c: aiger/aiger.i aiger/aiger.h
	swig -python aiger/aiger.i

.PHONY: clean all

all: task2aig aiger/_aiger_wrap.so

clean:
	rm -f task2aig
	rm aiger/_aiger_wrap.so aiger/aiger_wrap.py
