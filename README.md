# task2aig
This utility translates a single deterministic task transition system into an
equivalent AIGER representation. The main design choices are summarized below.
* Configurations are pairs of integers corresponding to the computation time
  already given to the current job as well as the time elapsed since the last
  job arrival.
* The number of latches used by the AIG is the required number of latches to
  encode both integer as well as 2 helper latches.
* The helper latches encode simple information: whether the initial job as
  already arrived and a 2-step ticker to allow for the scheduler to move before
  the environment.
* The inputs consist of controllable inputs encoding the index of the chosen
  task to give execution time and two uncontrollable inputs to determine
  whether the job can be considered finished and whether a new job should
  arrive.

## Parameters
* Number of tasks for the global system
* Task id (indexed from 1)
* Possible execution times (list of integers)
* Possible inter-arrival times (list of integers)
* Deadline
* Initial arrival time

# Encoding task systems with Python
There are two Python scripts provided to generate an AIG for your task systems
and to read the resulting safe region from an AIG.

## encodeTasks
This reads a task system from a file in the format proposed by S. Guha and
uses task2aig to generate the AIG input file.

## decodeSafe
If you are reading the safe region of the system encoded as an AIG, and you
plan on using the provided scripts, then you will need to make/build the 
SWIG interface for Python and C in the aiger subdirectory. There is a Makefile
in the subdirectory for your convenience. (We recommend that you change python
to python3 everywhere in Makefile if you are working with a 2.x version of
python as your default.) The safe region is assumed to be
encoded as an AIG for which all latches are inputs and a single output signals
whether the latch configuration is safe.
