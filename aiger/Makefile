aiger_wrap.c:
	swig -python aiger.i

_aiger_wrap.*.so:
	python setup.py build_ext --inplace

.PHONY: all clean

all: aiger_wrap.c _aiger_wrap.*.so

clean:
	rm aiger_wrap.c aiger_wrap.py _aiger_wrap.*.so
