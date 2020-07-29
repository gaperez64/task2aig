/**************************************************************************
 * Copyright (c) 2020- Guillermo A. Perez
 * 
 * AIGPROD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * AIGPROD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with AIGPROD. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Guillermo A. Perez
 * University of Antwerp
 * guillermoalberto.perez@uantwerpen.be
 *************************************************************************/

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "aiger/aiger.h"

static void printHelp() {
    fprintf(stderr, "Usage: aigprod INPUTFILES...\n");
    fprintf(stderr, "Create the product of AIGs with common inputs.\n");
    return;
}

static inline unsigned shift(unsigned lit, unsigned offset, unsigned inputs) {
    // 0 and 1 are not lits, so inputs gives us the maximal variable name
    // for a real literal. In AIGER, we hide negations between even numbers
    // so inputs * 2 is the real maximal non-negated literal index, we add
    // 1 to account for it appearing negated too
    unsigned ret;
    if (lit <= (inputs * 2) + 1)
        ret = lit;
    else
        ret = lit + offset;
    return ret;
}

/* Take and-inverter graphs with the same set of inputs
 * and construct their product
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Expected at least 2 input AIG files as arguments.\n");
        printHelp();
        return EXIT_FAILURE;
    }
    aiger* dst = aiger_init();
    unsigned inputs = UINT_MAX;
    unsigned offset = 0;
    unsigned output = 0;
    for (int srcidx = 1; srcidx < argc; srcidx++) {
        char* srcPath = argv[srcidx];
#ifndef NDEBUG
        fprintf(stderr, "Processing file: %s\n", srcPath);
#endif
        aiger* src = aiger_init();
        const char* err = aiger_open_and_read_from_file(src, srcPath);
        if (err) {
            fprintf(stderr, "Reading error on %s: %s\n",
                    srcPath, err);
            aiger_reset(src);
            aiger_reset(dst);
            return EXIT_FAILURE;
        }
        // for the first file we add the inputs and remember the number of
        // them
        if (inputs == UINT_MAX) {
            inputs = src->num_inputs;
            for (int i = 0; i < src->num_inputs; i++) {
                aiger_symbol sym = src->inputs[i];
                aiger_add_input(dst, sym.lit, sym.name);
            }
        } else {
            if (src->num_inputs != inputs) {
                fprintf(stderr, "%s: Expected %d inputs but got %d\n",
                        srcPath, inputs, src->num_inputs);
                aiger_reset(dst);
                aiger_reset(src);
                return EXIT_FAILURE;
            }
        }

        // add gates, latches, and outputs to dst
#ifndef NDEBUG
        fprintf(stderr, "Adding gates, etc. with offset %d\n", offset);
#endif
        for (int i = 0; i < src->num_ands; i++) {
            aiger_and and = src->ands[i]; 
            aiger_add_and(dst, shift(and.lhs, offset, inputs),
                          shift(and.rhs0, offset, inputs),
                          shift(and.rhs1, offset, inputs));
        }
        for (int i = 0; i < src->num_latches; i++) {
            aiger_symbol latch = src->latches[i];
            aiger_add_latch(dst, shift(latch.lit, offset, inputs),
                            shift(latch.next, offset, inputs),
                            latch.name);
        }
        assert(src->num_outputs == 1);
        aiger_symbol out = src->outputs[0];
        unsigned oldOutput = output;
        output = dst->maxvar * 2 + 2;
        // we construct the or of the old one with the new one
        // so we negate them, take an and
        aiger_add_and(dst, output, aiger_not(oldOutput),
                      aiger_not(shift(out.lit, offset, inputs)));
        // and consider its negation too
        output = aiger_not(output); 
        // clean up the aiger structure for this file
        aiger_reset(src);

        // update the offset
        offset = dst->maxvar * 2;
    }

    // add a final output
    aiger_add_output(dst, output, "output_disjunction");

#ifndef NDEBUG
    fprintf(stderr, "AIG structure created, now checking it!\n");
    const char* msg = aiger_check(dst);
    if (msg) {
        fprintf(stderr, "%s\n", msg);
    }
#endif

    // and dump the aig
    aiger_write_to_file(dst, aiger_ascii_mode, stdout);

    // Free dynamic memory
    aiger_reset(dst);
    return EXIT_SUCCESS;
}

