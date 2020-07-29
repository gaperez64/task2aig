#!/usr/bin/env python

"""
setup.py file for aiger
"""

from distutils.core import setup, Extension


aiger_module = Extension("_aiger_wrap", sources=["aiger_wrap.c", "aiger.c"])

setup(name="aiger",
      version="1.0",
      author="G. A. Perez",
      description="""A swig wrapper for A. Biere's aiger""",
      ext_modules=[aiger_module],
      py_modules=["aiger"])
