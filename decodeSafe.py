#!/usr/bin/env python3

import sys

from aiger.aiger_wrap import (
    aiger_init,
    aiger_open_and_read_from_file,
    aiger_reset,
    get_aiger_symbol,
)


class AIG(object):
    def iterate_inputs(self):
        for i in range(int(self.spec.num_inputs)):
            yield get_aiger_symbol(self.spec.inputs, i)

    def iterate_latches(self):
        for i in range(int(self.spec.num_latches)):
            yield get_aiger_symbol(self.spec.latches, i)

    def __init__(self, aiger_file_name):
        self.spec = aiger_init()
        err = aiger_open_and_read_from_file(self.spec, aiger_file_name)
        if err:
            print("Error: {}".format(err))
        latches = [x.lit for x in self.iterate_latches()]
        print("{} Latches: {}".format(str(len(latches)), str(latches)))
        inputs = [x.lit for x in self.iterate_inputs()]
        print("{} Inputs: {}".format(str(len(inputs)), str(inputs)))

    def __del__(self):
        if self.spec:
            aiger_reset(self.spec)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Expected an AIG file name as unique argument")
        exit(1)
    else:
        AIG(sys.argv[1])
        exit(0)
