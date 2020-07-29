/**************************************************************************
 * Copyright (c) 2020- Guillermo A. Perez
 * 
 * TASK2AIG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * TASK2AIG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with TASK2AIG. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Guillermo A. Perez
 * University of Antwerp
 * guillermoalberto.perez@uantwerpen.be
 *************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aiger/aiger.h"

/* A red-black tree to keep track of all and gates
 * we create for the transition function
 * Conventions:
 * (1) literals are positive or negative integers depending on whether they
 * are negated
 * (2) the key is composed of the left operand (literal) and the right operand
 * with the left operand being the "smaller" variable (not literal)
 * (3) variables are indexed from 2 (so 1 is "True" and -1 is "False")
 */
typedef enum {BLACK, RED} NodeColor;

typedef struct RBTree {
    struct RBTree* parent;
    struct RBTree* left;
    struct RBTree* right;
    NodeColor color;
    int opLeft;
    int opRight;
    int var;
} RBTree;

static char* colorStr(NodeColor c) {
    return c == RED ? "red" : "black";
}

static void recursivePrint(RBTree* n, int h) {
    // Essentially: a DFS with in-order printing
    if (n == NULL)
        return;
    recursivePrint(n->left, h + 1);
    fprintf(stderr, "%d: key=(%d,%d), var=%d (%s)\n", h, n->opLeft,
            n->opRight, n->var, colorStr(n->color));
    recursivePrint(n->right, h + 1);
}

static inline unsigned var2aiglit(int var) {
    if (var == -1)
        return 0;
    if (var == 1)
        return 1;
    int aiglit = 2 * (abs(var) - 1);
    if (var < 0)
        aiglit += 1;
    return aiglit;
}

static void dumpAiger(RBTree* n, aiger* aig) {
    // Essentially: a DFS with in-order dump
    if (n == NULL)
        return;
    dumpAiger(n->left, aig);
#ifndef NDEBUG
    fprintf(stderr, "Adding variable %d (%d)\n", var2aiglit(n->var), n->var);
    fprintf(stderr, "tis an and of %d (%d) with %d (%d)\n",
            var2aiglit(n->opLeft), n->opLeft,
            var2aiglit(n->opRight), n->opRight);
#endif
    aiger_add_and(aig,
                  var2aiglit(n->var),
                  var2aiglit(n->opLeft),
                  var2aiglit(n->opRight));
    dumpAiger(n->right, aig);
}

static void deleteTree(RBTree* n) {
    if (n == NULL)
        return;
    deleteTree(n->left);
    deleteTree(n->right);
    free(n);
}

static inline bool lessThan(RBTree* n, RBTree* m) {
    int nLeft = n->opLeft;
    int nRight = n->opRight;
    int mLeft = m->opLeft;
    int mRight = m->opRight;
    return (nLeft < mLeft) ||
           ((nLeft == mLeft)
            && (nRight < mRight));
}

static inline bool equalKeys(RBTree* n, RBTree* m) {
    return (n->opLeft == m->opLeft) && (n->opRight == m->opRight);
}

static RBTree* recursiveInsert(RBTree* root, RBTree* n) {
    // Essentially: go to the leaves and insert a red node
    if (root != NULL) {
        if (lessThan(n, root)) {
            if (root->left != NULL) {
                return recursiveInsert(root->left, n);
            } else {
                root->left = n;
            }
        } else if (equalKeys(n, root)) {
            return root;
        } else { // the root's key is strictly larger
            if (root->right != NULL) {
                return recursiveInsert(root->right, n);
            } else {
                root->right = n;
            }
        }
    }
    // insert n
    // NOTE: root may be NULL here
    n->parent = root;
    n->left = NULL;
    n->right = NULL;
    n->color = RED;
    return n;
}

static inline void attachToParent(RBTree* n, RBTree* p, RBTree* nnew) {
    if (p != NULL) {
        if (n == p->left) {
            p->left = nnew;
        } else if (n == p->right) {
            p->right = nnew;
        }
    }
    nnew->parent = p;
}

static void rotateLeft(RBTree* n) {
    RBTree* nnew = n->right;
    RBTree* p = n->parent;
    assert(nnew != NULL);  // Since the leaves of a red-black tree are empty,
                           // they cannot become internal nodes.
    n->right = nnew->left;
    nnew->left = n;
    n->parent = nnew;
    // handle other child/parent pointers
    if (n->right != NULL)
        n->right->parent = n;

    // attach to parent if n is not the root
    attachToParent(n, p, nnew);
}

static void rotateRight(RBTree* n) {
    RBTree* nnew = n->left;
    RBTree* p = n->parent;
    assert(nnew != NULL);  // Since the leaves of a red-black tree are empty,
                           // they cannot become internal nodes.
    n->left = nnew->right;
    nnew->right = n;
    n->parent = nnew;
    // handle other child/parent pointers
    if (n->left != NULL)
        n->left->parent = n;

    // attach to parent if n is not the root
    attachToParent(n, p, nnew);
}

static inline RBTree* sibling(RBTree* n) {
    if (n->parent == NULL) {
        return NULL;
    } else {
        return n == n->parent->left ? n->parent->right : n->parent->left;
    }
}

static inline RBTree* uncle(RBTree* n) {
    if (n->parent == NULL) {
        return NULL;
    } else {
        return sibling(n->parent);
    }
}

static void repairRBTree(RBTree* n) {
    if (n->parent == NULL) {
        // the root must be black
        n->color = BLACK;
    } else if (n->parent->color == BLACK) {
        // do nothing, all properties hold
    } else if (uncle(n) != NULL && uncle(n)->color == RED) {
        // we swap colors of parent & uncle with their parent
        n->parent->color = BLACK;
        uncle(n)->color = BLACK;
        n->parent->parent->color = RED;
        // and recurse on the grandparent to fix potential problems
        repairRBTree(n->parent->parent);
    } else {
        // we now want to rotate n with its grandparent, but for this to work
        // we need n to be either the left-left grandchild or the right-right
        // grandchild
        RBTree* p = n->parent;
        RBTree* g = p->parent;
        // we start by rotating n with its parent, if needed,
        // to guarantee this property
        if (n == p->right && p == g->left) {
            rotateLeft(p);
            n = n->left;
        } else if (n == p->left && p == g->right) {
            rotateRight(p);
            n = n->right;
        }
        // now we can rotate with the grandparent, and swap the colors of
        // parent and grandparent
        p = n->parent;
        g = p->parent;
        if (n == p->left) {
            rotateRight(g);
        } else {
            rotateLeft(g);
        }
        p->color = BLACK;
        g->color = RED;
    }
}

static RBTree* insertNode(RBTree* root, int op1, int op2, int freshVar) {
    RBTree* n;
    n = malloc(sizeof(RBTree));
    if (abs(op1) <= abs(op2)) {
        n->opLeft = op1;
        n->opRight = op2;
    } else {
        n->opLeft = op2;
        n->opRight = op1;
    }
    n->var = freshVar;
    n->parent = NULL;
    // the recursive insertion fails if a node with the
    // same key exists already
    RBTree* m = recursiveInsert(root, n);
    // in that case m is the node in the tree with the
    // same key but different variable
    if (m->var != n->var) {
        free(n);
        return m;
    }
    // repair the tree to recover red-black properties
    repairRBTree(n);
    return n;
}

static inline RBTree* findRoot(RBTree* n) {
    // find the new root
    RBTree* root = n;
    while (root->parent != NULL)
        root = root->parent;
    return root;
}

typedef struct {
    RBTree* root;
    int nextVar;
} AigTable;

static inline int and(AigTable* table, int op1, int op2) {
    assert(op1 != 0 && op2 != 0);
    RBTree* t = insertNode(table->root, op1, op2, table->nextVar);
    if (t->var == table->nextVar) {
        table->nextVar++;
        table->root = findRoot(t);
    }
    return t->var;
}

static inline int or(AigTable* table, int op1, int op2) {
    return -1 * and(table, -1 * op1, -1 * op2);
}

static inline int getBin(AigTable* table, int n, int start, int end) {
    int mask = 1;
    int ret = 1;
    for (int var = start; var < end; var++) {
        if ((n & mask) == mask)
            ret = and(table, ret, var);
        else
            ret = and(table, ret, var * -1);
        mask = mask << 1;
    }
    return ret;
}

/* Encode the single-task system in and-inverter
 * graphs, then use A. Biere's AIGER to dump the graph
 */
void encodeTask(int notasks, int index, int deadline, int init,
                int noExecTimes, int* exectimes,
                int noArrivalTimes, int* arrivaltimes) {
#ifndef NDEBUG
    fprintf(stderr, "Number of tasks = %d\n", notasks);
    fprintf(stderr, "Index of task = %d\n", index);
    fprintf(stderr, "Deadline = %d\n", deadline);
    fprintf(stderr, "Initial arrival = %d\n", init);
    fprintf(stderr, "Possible execution times: ");
    for (int i = 0; i < noExecTimes; i++)
        fprintf(stderr, "%d ", exectimes[i]);
    fprintf(stderr, "\nPossible arrival times: ");
    for (int i = 0; i < noArrivalTimes; i++)
        fprintf(stderr, "%d ", arrivaltimes[i]);
    fprintf(stderr, "\n");
#endif

    // We now encode the transition relation into our
    // "sorta unique" AIG symbol table
    AigTable andGates;
    andGates.root = NULL;
    andGates.nextVar = 2;

    // We need to reserve a few variables though
    // (1) one per controllable input + 2 uncontrollable inputs
    // (2) one per latch needed for the counters + 2 helpers
    // we need floor(lg(notasks)) + 1 just for controllable inputs,
    // where lg is the logarithm base 2; but C only has
    // natural logarithms
    int noInputs = (int) (log(notasks) / log(2.0)) + 1;
    noInputs += 2;  // uncontrollable inputs
    andGates.nextVar += noInputs;
    // we will also have 2 counters encoded in binary and 2 helper latches
    int noExecLatches = (int) (log(exectimes[noExecTimes - 1]) / log(2.0)) + 1;
    // arrival times are never fully counted (i.e. the counter stays strictly
    // below the max) so we try to reduce latches using that fact
    int noArrivalLatches = (int) (log(arrivaltimes[noArrivalTimes - 1] - 1) / log(2.0)) + 1;
    int noLatches = noExecLatches + noArrivalLatches + 2;
    andGates.nextVar += noLatches;
    // we will be using all counter latches for the initialization countdown
    assert((int) (log(init) / log(2.0)) + 1 <= noLatches);
#ifndef NDEBUG
    fprintf(stderr, "Reserved %d inputs\n", noInputs);
    fprintf(stderr, "Reserved %d latches\n", noLatches);
#endif

    // Step 1: set up choice decoder
    int taskScheduled = getBin(&andGates, index, 2, 2 + noInputs - 2);

    // Step 2: set up initialization counter and logic for initialization
    // latch
    // 2.1 counter logic before initialization
    int latchFunction[noLatches - 2];
    for (int i = 0; i < noLatches - 2; i++) {
        latchFunction[i] = -1;
    }
    // we set the bit to 1 if it is 0, all less significant bits are 1,
    // and the tick_tock clock is set to 1; or if it is 1 and either
    // the tick_tock clock is 0 or some less significant bit is not 1,
    // additionally the initialization is not yet done
    int rollingLSB = 1;
    const int ticktockLatch = 2 + noInputs + noLatches - 2;
    const int initdLatch = 2 + noInputs + noLatches - 1;
    for (int i = 0; i < noLatches - 2; i++) {
        int latchvar = 2 + noInputs + i;
        int flip = and(&andGates, latchvar * -1,
                       and(&andGates, ticktockLatch, rollingLSB));
        int keep = and(&andGates, latchvar,
                       or(&andGates, ticktockLatch * -1, rollingLSB * -1));
        latchFunction[i] = or(&andGates, latchFunction[i],
                              or(&andGates, flip, keep));
        rollingLSB = and(&andGates, rollingLSB, latchvar);
    }
    // 2.1: logic for the initialization latch
    int mask = 1;
    int isInitialized = 1;
    for (int i = 0; i < noLatches - 2; i++) {
        if ((init & mask) == mask)
            isInitialized = and(&andGates, isInitialized, latchFunction[i]);
        else
            isInitialized = and(&andGates, isInitialized, latchFunction[i] * -1);
        mask = mask << 1;
    }
    // if it is initialized already, keep it that way
    isInitialized = or(&andGates, isInitialized, initdLatch);
    // 2.3: we update pre-init counter logic to guard the updated with this
    for (int i = 0; i < noLatches - 2; i++)
        latchFunction[i] = and(&andGates, latchFunction[i], isInitialized * -1);

    // Step 3: Arrival time counter logic
    // we set the bit to 1 if it is 0, all less significant bits are 1, the
    // tick_tock clock is set to 1; or if it is 1 and
    // either the tick_tock clock is 0 or some less significant bit is not 1
    // NOTE: this is all guarded by initialization and non-arrival
    int canArrive = -1;
    for (int i = 0; i < noArrivalTimes - 1; i++) {
        int arrivalAllowed = getBin(&andGates, arrivaltimes[i] - 1,
                                    2 + noInputs + noExecLatches,
                                    2 + noInputs+ noExecLatches + noArrivalLatches);
        canArrive = or(&andGates, canArrive, arrivalAllowed);
    }
    int mustArrive = getBin(&andGates, arrivaltimes[noArrivalTimes - 1] - 1,
                            2 + noInputs + noExecLatches,
                            2 + noInputs + noExecLatches + noArrivalLatches);
    const int nextJobInput = 2 + noInputs - 1;
    int newJob = and(&andGates, canArrive, nextJobInput);
    newJob = or(&andGates, newJob, mustArrive);
    newJob = and(&andGates, newJob, ticktockLatch);
    int guard = and(&andGates, isInitialized, newJob * -1);
    // the guard is ready, we can set up the counter logic now
    rollingLSB = 1;
    for (int i = noExecLatches; i < noExecLatches + noArrivalLatches; i++) {
        int latchvar = 2 + noInputs + i;
        int flip = and(&andGates, latchvar * -1,
                       and(&andGates, ticktockLatch, rollingLSB));
        int keep = or(&andGates, ticktockLatch * -1, rollingLSB * -1);
        keep = and(&andGates, latchvar, keep);
        latchFunction[i] = or(&andGates, latchFunction[i],
                              and(&andGates, guard,
                                  or(&andGates, flip, keep)));
        rollingLSB = and(&andGates, rollingLSB, latchvar);
    }

    // Step 4: Execution time counter logic
    // we set the bit to 1 if it is 0, all less significant bits are 1, the
    // tick_tock clock is set to 0, and the index is right; or if it is 1 and
    // either the tick_tock clock is 1 or some less significant bit is not 1
    // or if the index is not right; or everything is 1 already; or if
    // execution terminates
    // NOTE: this is all guarded by initialization and non-arrival
    // and non-termination
    int canTerminate = -1;
    for (int i = 0; i < noExecTimes - 1; i++) {
        int termAllowed = getBin(&andGates, exectimes[i],
                                 2 + noInputs,
                                 2 + noInputs + noExecLatches);
        canTerminate = or(&andGates, canTerminate, termAllowed);
    }
    int mustTerminate = getBin(&andGates, exectimes[noExecTimes - 1],
                               2 + noInputs,
                               2 + noInputs + noExecLatches);
    const int endExecInput = 2 + noInputs - 2;
    int endExec = and(&andGates, canTerminate, endExecInput);
    endExec = or(&andGates, endExec, mustTerminate);
    endExec = and(&andGates, endExec, ticktockLatch);
    // the endExec flag will be used to set all bits to 1
    int allset = 1;
    for (int i = 0; i < noExecLatches; i++) {
        int latchvar = 2 + noInputs + i;
        allset = and(&andGates, allset, latchvar);
    }
    allset = or(&andGates, allset, endExec);
    rollingLSB = 1;
    for (int i = 0; i < noExecLatches; i++) {
        int latchvar = 2 + noInputs + i;
        int flip = and(&andGates, latchvar * -1, taskScheduled);
        flip = and(&andGates, flip,
                   and(&andGates, ticktockLatch * -1, rollingLSB));
        int keep = or(&andGates, ticktockLatch,
                      or(&andGates, taskScheduled * -1, rollingLSB * -1));
        keep = and(&andGates, latchvar, keep);
        latchFunction[i] = or(&andGates, latchFunction[i],
                              and(&andGates, guard,
                                  or(&andGates, allset,
                                     or(&andGates, flip, keep))));
        rollingLSB = and(&andGates, rollingLSB, latchvar);
    }

    // Step 5: Deadline check with execution timer
    mask = 1;
    int atDeadline = getBin(&andGates, deadline,
                            2 + noInputs + noExecLatches,
                            2 + noInputs + noExecLatches + noArrivalLatches);
    int unsafe = and(&andGates, atDeadline, allset * -1);
    unsafe = and(&andGates, unsafe, ticktockLatch);

#ifndef NDEBUG
    recursivePrint(andGates.root, 0);
#endif

    // Step 6: Create and print the constructed AIG
    aiger* aig = aiger_init();

    // add inputs
    int lit = 2;
    char name[50];
    for (int i = 0; i < noInputs - 2; i++) {
        sprintf(name, "controllable_choicetask%d", i);
        aiger_add_input(aig, lit, name);
        lit += 2;
    }
    aiger_add_input(aig, lit, "end_exec_early");
    lit += 2;
    aiger_add_input(aig, lit, "next_job");
    lit += 2;

    // add latches
    for (int i = 0; i < noExecLatches; i++) {
        sprintf(name, "exec_counter_latch%d", i);
        aiger_add_latch(aig, lit, var2aiglit(latchFunction[i]), name);
        lit += 2;
    }
    for (int i = 0; i < noArrivalLatches; i++) {
        sprintf(name, "arrival_counter_latch%d", i);
        aiger_add_latch(aig, lit, var2aiglit(latchFunction[noExecLatches + i]), name);
        lit += 2;
    }
    // we add the latch to keep track of odd/even ticks
    aiger_add_latch(aig, lit, lit + 1, "tick_tock");
    lit += 2;
    // we add the latch to keep track of whether we are initialized
    aiger_add_latch(aig, lit, var2aiglit(isInitialized), "is_initialized");
    lit += 2;

    // add and-gates
#ifndef NDEBUG
    fprintf(stderr, "Dumping AND-gates into aiger structure\n");
#endif
    dumpAiger(andGates.root, aig);

    // add bad state
    aiger_add_output(aig, var2aiglit(unsafe), "missed_deadline");

#ifndef NDEBUG
    fprintf(stderr, "AIG structure created, now checking it!\n");
    const char* msg = aiger_check(aig);
    if (msg) {
        fprintf(stderr, "%s\n", msg);
    }
#endif

    // and dump the aig
    aiger_write_to_file(aig, aiger_ascii_mode, stdout);

    // Free dynamic memory
    aiger_reset(aig);
    deleteTree(andGates.root);
    return;
}

static void printHelp() {
    fprintf(stderr, "Usage: task2aig [OPTIONS]... TOTTASKS TASKINDEX DEADLINE "
                    "INITARRIVAL MAXEXECTIME MAXARRIVALTIME\n");
    fprintf(stderr, "Create an AIG for a deterministic task system.\n");
    fprintf(stderr, "  -h    print this message\n");
    fprintf(stderr, "  -e    possible execution time, multiple allowed\n");
    fprintf(stderr, "  -a    possible arrival time, multiple allowed\n");
    return;
}

typedef struct SLIntList {
    int val;
    struct SLIntList* next;
} SLIntList;

void deleteSLIntList(SLIntList* item) {
    while (item != NULL) {
        SLIntList* temp = item->next;
        free(item);
        item = temp;
    }
    return;
}

int main(int argc, char* argv[]) {
    int c;
    int notasks;
    int index;
    int deadline;
    int init;
    int maxexec;
    int maxarrival;
    SLIntList* execTimes = NULL;
    SLIntList* arrivalTimes = NULL;
    int noExecTimes = 1;
    int noArrivalTimes = 1;
    int x;
    while ((c = getopt(argc, argv, "he:a:")) != -1) {
        switch (c) {
            case 'h':
                printHelp();
                break;
            case 'e':
                x = atoi(optarg);
                if (execTimes == NULL || execTimes->val > x) {
                    SLIntList* temp = execTimes;
                    execTimes = malloc(sizeof(SLIntList));
                    execTimes->next = temp;
                    execTimes->val = atoi(optarg);
                    noExecTimes++;
                } else {
                    SLIntList* item = execTimes;
                    while (item->next != NULL && item->next->val <= x)
                        item = item->next;
                    if (item->val < x) {
                        SLIntList* temp = item->next;
                        item->next = malloc(sizeof(SLIntList));
                        item->next->val = x;
                        item->next->next = temp;
                        noExecTimes++;
                    }
                }
                break;
            case 'a':
                x = atoi(optarg);
                if (arrivalTimes == NULL || arrivalTimes->val > x) {
                    SLIntList* temp = arrivalTimes;
                    arrivalTimes = malloc(sizeof(SLIntList));
                    arrivalTimes->next = temp;
                    arrivalTimes->val = atoi(optarg);
                    noArrivalTimes++;
                } else {
                    SLIntList* item = arrivalTimes;
                    while (item->next != NULL && item->next->val <= x)
                        item = item->next;
                    if (item->val < x) {
                        SLIntList* temp = item->next;
                        item->next = malloc(sizeof(SLIntList));
                        item->next->val = x;
                        item->next->next = temp;
                        noArrivalTimes++;
                    }
                }
                break;
            case '?':  // getopt found an invalid option
                // get rid of dynamic memory and exit
                deleteSLIntList(execTimes);
                deleteSLIntList(arrivalTimes);
                return EXIT_FAILURE;
            default:
                assert(false);  // this should not be reachable
        }
    }

    // making sure we have precisely 6 non-options
    if (argc - optind != 6) {
        fprintf(stderr, "Expected 6 positional arguments!\n");
        // get rid of dynamic memory and exit
        deleteSLIntList(execTimes);
        deleteSLIntList(arrivalTimes);
        return EXIT_FAILURE;
    } else {
        notasks = atoi(argv[optind++]);
        index = atoi(argv[optind++]);
        deadline = atoi(argv[optind++]);
        init = atoi(argv[optind++]);
        maxexec = atoi(argv[optind++]);
        maxarrival = atoi(argv[optind++]);
    }

    // create arrays for time lists
    int execarray[noExecTimes];
    SLIntList* item = execTimes;
    for (int i = 0; i < noExecTimes - 1; i++) {
        assert(item != NULL);
        execarray[i] = item->val;
        item = item->next;
    }
    execarray[noExecTimes - 1] = maxexec;
    int arrivalarray[noArrivalTimes];
    item = arrivalTimes;
    for (int i = 0; i < noArrivalTimes - 1; i++) {
        assert(item != NULL);
        arrivalarray[i] = item->val;
        item = item->next;
    }
    arrivalarray[noArrivalTimes - 1] = maxarrival;

    // get rid of dynamic memory
    deleteSLIntList(execTimes);
    deleteSLIntList(arrivalTimes);

    // encode the task as an and-inverter graph
    encodeTask(notasks, index, deadline, init,
               noExecTimes, execarray, noArrivalTimes, arrivalarray);

    return EXIT_SUCCESS;
}
