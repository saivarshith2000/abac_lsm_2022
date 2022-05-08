# Script to evaluate performance of ABAC LSM
import sys
import json
import os
import statistics
from time import perf_counter_ns

KERNEL_MODELS = ['abac_trees', 'abac_trees_enc', 'abac_rules', 'abac_rules_enc'] 
ITERATIONS = 50

def load_data(u_path, k_path):
    # Loads data in file u_path to securityfs file at k_path
    try:
        with open(u_path) as f:
            data = f.read()
        with open(k_path, 'w') as f:
            f.write(data)
    except Exception as e:
        sys.exit(f"The following error occured while loading data from {u_path} to {k_path}\n{e}")

def start_recording():
    # Start recording access times in kernel module
    with open('/sys/kernel/security/abac/action', 'w') as f:
        f.write("RECORD")
    #print("Kernel recording started...")

def stop_recording():
    # stop recording access times in kernel module
    with open('/sys/kernel/security/abac/action', 'w') as f:
        f.write("STOP")
    #print("Kernel recording stopped...")


def get_kernel_time():
    # get access times measured inside kernel module
    with open('/sys/kernel/security/abac/perf') as f:
        t = f.read()
    # t contains the time in nanoseconds followed by newline
    t = t.strip()
    # Return time in milliseconds
    #return int(t)/1000000
    return int(t)

def load_into_kernel(data_path, kernel_model):
    print("\nLoading data...")
    if kernel_model == "abac_trees":
        load_data(f'{data_path}/trees/original/user_attr', '/sys/kernel/security/abac/user_attr')
        load_data(f'{data_path}/trees/original/env_attr', '/sys/kernel/security/abac/env_attr')
        load_data(f'{data_path}/trees/original/obj_attr', '/sys/kernel/security/abac/obj_attr')
    elif kernel_model == "abac_trees_enc":
        load_data(f'{data_path}/trees/encoded/user_attr', '/sys/kernel/security/abac/user_attr')
        load_data(f'{data_path}/trees/encoded/env_attr', '/sys/kernel/security/abac/env_attr')
        load_data(f'{data_path}/trees/encoded/obj_attr', '/sys/kernel/security/abac/obj_attr')
    elif kernel_model == "abac_rules":
        load_data(f'{data_path}/rules/original/user_attr', '/sys/kernel/security/abac/user_attr')
        load_data(f'{data_path}/rules/original/env_attr', '/sys/kernel/security/abac/env_attr')
        load_data(f'{data_path}/rules/original/obj_rules', '/sys/kernel/security/abac/obj_rules')
        load_data(f'{data_path}/rules/original/policy', '/sys/kernel/security/abac/policy')
    elif kernel_model == "abac_rules_enc":
        load_data(f'{data_path}/rules/encoded/user_attr', '/sys/kernel/security/abac/user_attr')
        load_data(f'{data_path}/rules/encoded/env_attr', '/sys/kernel/security/abac/env_attr')
        load_data(f'{data_path}/rules/encoded/obj_rules', '/sys/kernel/security/abac/obj_rules')
        load_data(f'{data_path}/rules/encoded/policy', '/sys/kernel/security/abac/policy')
    else:
        sys.exit(f"invalid kernel model\nValid models - {KERNEL_MODELS}")
    print("\nData loaded into the kernel")

def main(config_path, data_path, kernel_model, dac_only=False):
    with open(config_path) as f:
        config = json.load(f)

    users = config['user']['count']
    obj = config['obj']['count']

    # load data into kernel
    if not dac_only:
        load_into_kernel(data_path, kernel_model)

    # execute access requests
    print(f"User Id - {os.getuid()}")
    print(f"Objects - {obj}")
    print(f"Iterations: {ITERATIONS}")
    allowed = 0
    denied = 0
    user_time = []
    kernel_time = []
    # Turn on recording mode in kernel
    if not dac_only:
        start_recording()
    for i in range(ITERATIONS):
        if i % 10 == 0:
            print(f"Iteration-{i}")
        path_nums = list(range(obj))
        random.shuffle(path_nums)
        for j in path_nums:
            filename = f"/home/secured/obj_{j}"
            start = perf_counter_ns()
            try:
                with open(filename, 'r') as f:
                    f.read()
                allowed += 1
            except PermissionError:
                denied += 1
            except Exception as e:
                sys.exit(f"Evaluation Failed. Please fix this error\n{e}")
            end = perf_counter_ns()
            # User time is counted per access
            user_time.append(end - start)
            # get kernel time 
            kernel_time.append(get_kernel_time())
    # Turn off recording mode in kernel
    if not dac_only:
        stop_recording()

    # performance statistics
    stats = {}

    stats['config'] = config['name']
    stats['num_requests'] = obj * ITERATIONS
    stats['allowed'] = allowed
    stats['denied'] = denied

    stats['user_time_mean'] = statistics.mean(user_time)
    stats['user_time_median'] = statistics.median(user_time)
    stats['user_time_pop_std'] = statistics.pstdev(user_time)
    stats['user_time_pvariance'] = statistics.pvariance(user_time)
    stats['user_total_time'] = sum(user_time)

    if not dac_only:
        stats['kernel_time_mean'] = statistics.mean(kernel_time)
        stats['kernel_time_median'] = statistics.median(kernel_time)
        stats['kernel_time_pop_std'] = statistics.pstdev(kernel_time)
        stats['kernel_time_pvariance'] = statistics.pvariance(kernel_time)
        stats['kernel_total_time'] = sum(kernel_time)

    print(f"len(user_time): {len(user_time)}")
    print(f"len(kernel_time): {len(kernel_time)}")
    print(f"Allowed: {allowed}")
    print(f"Denied: {denied}")
    print(f"Access Requests: {stats['num_requests']}")

    # Print user statistics
    print(f"User Total Time: {stats['user_total_time']:.20f} ns")
    print(f"User Time Mean: {stats['user_time_mean']:.20f} ns")
    print(f"User Time Median: {stats['user_time_median']:.20f} ns")
    print(f"User Time Standard Deviation: {stats['user_time_pop_std']:.20f} ns")
    print(f"User Time Variance: {stats['user_time_pvariance']:.20f} ns")
    print()

    # Print kernel statistics
    if not dac_only:
        print(f"Kernel Total Time: {stats['kernel_total_time']:.20f} ns")
        print(f"Kernel Time Mean: {stats['kernel_time_mean']:.20f} ns")
        print(f"Kernel Time Median: {stats['kernel_time_median']:.20f} ns")
        print(f"Kernel Time Standard Deviation: {stats['kernel_time_pop_std']:.20f} ns")
        print(f"Kernel Time Variance: {stats['kernel_time_pvariance']:.20f} ns")

    return stats


if __name__ == "__main__":
    if len(sys.argv) < 4:
        sys.exit(f"Invalid Usage\npython3 {sys.argv[0]} <config_path> <data_path> <kernel_model> <dac_only>\
                \nKernel model is the ABAC lsm that is currently loaded. Must be one of abac_trees, abac_trees_enc, abac_rules, abac_rules_enc")
    if len(sys.argv) == 5:
        main(sys.argv[1], sys.argv[2], sys.argv[3], True)
    else:
        main(sys.argv[1], sys.argv[2], sys.argv[3])
