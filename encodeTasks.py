#!/usr/bin/env python3

import sys
import subprocess
from fractions import Fraction
from decimal import Decimal


def get_tasks(file_name):
    f = open(file_name, "r")

    # readlines reads the individual line into a list
    fl = f.readlines()
    lines = []
    tasks = []
    hard_tasks = []
    soft_tasks = []
    has_soft_tasks = False

    f.close()

    # for each line, extract the task parameters

    for x in fl:
        y = x.strip()

        if y[0] == '-':
            hard_tasks = tasks
            tasks = []
            has_soft_tasks = True

        elif y != '' and y[0] != '#':
            lines.append(y)
            task = y.split("|")  # task should have 4 elements

            arrival = int(task[0])

            dist = task[1].split(";")  # distribution on the execution time
            exe = []
            max_exe_time = 0
            for z in dist:
                z = z.strip("[")
                z = z.strip("]")
                z = z.split(",")
                time = int(z[0])

                if time > max_exe_time:  # compute maximum execution time
                    max_exe_time = time

                exe.append((time, Fraction(Decimal(z[1]))))

            deadline = int(task[2])

            dist = task[3].split(";")  # distribution on the period
            period = []
            arrive_time = []
            for z in dist:
                z = z.strip("[")
                z = z.strip("]")
                z = z.split(",")
                time = int(z[0])
                period.append((time, Fraction(Decimal(z[1]))))

                arrive_time.append(time)

            min_arrive_time = min(arrive_time)

            if has_soft_tasks:
                cost = Fraction(Decimal(task[4]))
                tasks.append([arrival, exe, deadline, period,
                              max_exe_time, min_arrive_time, cost])
            else:
                tasks.append([arrival, exe, deadline, period,
                              max_exe_time, min_arrive_time])

    if has_soft_tasks:
        soft_tasks = tasks
    else:
        hard_tasks = tasks

    return hard_tasks, soft_tasks


def encode(file_name):
    hard_tasks, _ = get_tasks(file_name)
    # hard_tasks and soft_tasks are a list of task descriptions:
    # [arrival, exe dist, deadline, period dist, max_exe_time, min_arrive_time]
    print("Found {} hard tasks".format(len(hard_tasks)))
    print("Proceeding to encode them into AIGER")
    i = 1
    for (init_arrival, exe_dist, deadline, period_dist, _, _) in hard_tasks:
        exec_times = sorted([x for (x, _) in exe_dist])
        arrival_times = sorted([x for (x, _) in period_dist])
        args = [len(hard_tasks), i, deadline,
                init_arrival, exec_times[-1],
                arrival_times[-1]]
        extra_exec = ["-e {}".format(e) for e in exec_times[:-1]]
        extra_arrival = ["-a {}".format(a) for a in arrival_times[:-1]]
        args = [str(a) for a in extra_exec + extra_arrival + args]

        # Call the aig encoder and save the aig to a temporary file
        completed = subprocess.run(["./task2aig"] + args,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        if completed.returncode != 0:
            print("An error occurred: {}".format(str(completed.stderr)))
            return completed.returncode
        f = open("temp{}.aag".format(i), "wb")
        f.write(completed.stdout)
        f.close()
        i += 1
    temp_files = ["temp{}.aag".format(i)
                  for i in range(1, len(hard_tasks) + 1)]
    print("We now have an AIGER for each task")
    print("Joining the files: {}".format(", ".join(temp_files)))
    completed = subprocess.run(["./aigprod"] + temp_files,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    if completed.returncode != 0:
        print("An error occurred: {}".format(str(completed.stderr)))
        return completed.returncode
    f = open("tasks.aag", "wb")
    f.write(completed.stdout)
    f.close()


def main():
    if len(sys.argv) != 2:
        print("Expected a task-system file name as unique argument")
        exit(1)
    else:
        file_name = sys.argv[1]
        file_name = file_name.strip()
        exit(encode(file_name))


if __name__ == "__main__":
    main()
