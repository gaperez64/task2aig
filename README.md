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

## Python script
If you are using the Python script to create an AIG for a system of several
tasks then you will need aigjoin and aigor from A. Biere's aiger tools.
