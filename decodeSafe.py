#!/usr/bin/env python3

import math
import sys

from aiger.aiger_wrap import (
    aiger_init,
    aiger_open_and_read_from_file,
    aiger_reset,
    aiger_is_and,
    get_aiger_symbol,
)


def strip_lit(lit):
    return lit & ~1


def is_negated(lit):
    return (lit & 1) == 1


class AIG(object):
    def is_and(self, lit):
        stripped = strip_lit(lit)
        return aiger_is_and(self.aig, stripped)

    def iterate_inputs(self):
        for i in range(int(self.aig.num_inputs)):
            yield get_aiger_symbol(self.aig.inputs, i)

    def iterate_latches(self):
        for i in range(int(self.aig.num_latches)):
            yield get_aiger_symbol(self.aig.latches, i)

    def __init__(self, aiger_file_name):
        self.aig = aiger_init()
        err = aiger_open_and_read_from_file(self.aig, aiger_file_name)
        if err:
            print("Error: {}".format(err))

    def __del__(self):
        if self.aig:
            aiger_reset(self.aig)

    def eval(self, lit, val):
        # We can focus on positive cases because...
        if is_negated(lit):
            stripped = strip_lit(lit)
            return not self.eval(stripped, val)

        a = self.is_and(lit)
        # We first deal with the leaves
        if not a:
            if lit == 0:
                return False
            else:
                var = lit // 2
                return val[var]
        # We can now deal with the inner nodes
        return self.eval(a.rhs0, val) and self.eval(a.rhs1, val)

    def decode(self, exec_times, arrival_times,
               max_exec_times, max_arrival_times,
               sched_task):
        # I am assuming there are only ands and inputs
        assert self.aig.num_latches == 0, "Did not expect latches"
        # Other sanity checks
        assert len(exec_times) == len(arrival_times),\
            "Different number of execution and arrival times"
        assert len(max_exec_times) == len(max_arrival_times),\
            "Different number of maximal times"
        assert len(exec_times) == len(max_exec_times),\
            "Exec and maximal exec times differ"
        exec_latches = [math.floor(math.log(m, 2)) + 1
                        for m in max_exec_times]
        arrival_latches = [math.floor(math.log(m - 1, 2)) + 1
                           for m in max_arrival_times]
        no_latches = sum(exec_latches) + sum(arrival_latches)
        aig_latches = self.aig.num_inputs - (2 * len(exec_times))
        assert no_latches == aig_latches,\
            "The AIG uses a different number of exec/arrival times"
        upd_exec_times = exec_times[:]
        if sched_task in range(len(exec_times)):
            upd_exec_times[sched_task] = min(max_exec_times[sched_task],
                                             exec_times[sched_task] + 1)
        upd_arrival_times = [t + 1 for t in arrival_times]
        # Now that we have the times after taking the proposed action,
        # we need to encode them as in the AIG: in binary + two extra
        # bits which we fix to 1 1
        bit_strings = []
        for i in range(len(exec_times)):
            s = "11"
            s += ("{0:0" + str(arrival_latches[i]) +
                  "b}").format(upd_arrival_times[i])
            s += ("{0:0" + str(exec_latches[i]) +
                  "b}").format(upd_exec_times[i])
            bit_strings.append(s)
        valuation = "".join([s[::-1] for s in bit_strings])
        assert self.aig.num_inputs == len(valuation),\
            "{} inputs and {} computed values".format(
                self.aig.num_inputs, len(valuation))
        # We can now use the valuation to recursively evaluate the output
        assert self.aig.num_outputs == 1, "Expected a single output"
        out = get_aiger_symbol(self.aig.outputs, 0)
        return self.eval(out.lit,
                         dict(zip([x.lit // 2 for x in self.iterate_inputs()],
                                  [bit == '1' for bit in valuation])))

    def __str__(self):
        ret_str = ""
        inputs = [x.lit for x in self.iterate_inputs()]
        ret_str += "{} Inputs: {}".format(len(inputs), inputs)
        return ret_str


def main():
    if len(sys.argv) != 2:
        print("Expected an AIG file name as unique argument")
        exit(1)
    else:
        aig = AIG(sys.argv[1])
        exit(aig.decode([0, 1, 3], [8, 6, 10], [2, 2, 3], [9, 10, 12], 2))


if __name__ == "__main__":
    main()
