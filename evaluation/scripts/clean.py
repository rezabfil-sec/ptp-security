import os
import numpy as np
import seaborn as sns
from matplotlib import pyplot as plt

# ------------------ ADJUST ME ------------------
repo_path = "/path/to/repo/ptp-security"
DEBUG = False # set to True to see the effects of the data cleaning
# -----------------------------------------------

file_path = "/node-6_abe/phc_cmp_processed.out"

hops = {
    'e2e': [
        {'num': 4, 'seq': 0},
        {'num': 5, 'seq': 5},
        {'num': 6, 'seq': 0},
        {'num': 7, 'seq': 4},
        {'num': 8, 'seq': 9},
        {'num': 9, 'seq': 14},
    ],
    'p2p': [
        {'num': 4, 'seq': 0},
        {'num': 5, 'seq': 5},
        {'num': 6, 'seq': 10},
        {'num': 7, 'seq': 15},
        {'num': 8, 'seq': 20},
        {'num': 9, 'seq': 25},
    ],
    'tc': [
        {'num': 4, 'seq': 0},
        {'num': 5, 'seq': 1},
        {'num': 6, 'seq': 0},
        {'num': 7, 'seq': 4},
        {'num': 8, 'seq': 9},
        {'num': 9, 'seq': 14},
    ],
}

types = ['e2e', 'p2p', 'tc']

algos = [   
    {'path': 'nosec', 'name': 'No Security'},
    {'path': 'hmacsha512256', 'name': 'HMAC-SHA-512-256'},
    {'path': 'blake2b', 'name': 'Blake2b'},
    {'path': 'blake3', 'name': 'Blake3b'},
    {'path': 'dummyr1ac', 'name': 'Dummy'},
]

def get_path(hops, measurement, algo, meas_type):
    return f"{repo_path}/evaluation/data/measurements-{meas_type}/" + \
        f"{hops}-hops/{measurement}_net-m-{hops}_stack-maggie-gm-{hops}-hops_action-1_{algo}" + \
        file_path

def get_path_out(hops, measurement, algo, meas_type):
    return f"{repo_path}/evaluation/data/measurements-{meas_type}-cleaned/" + \
        f"{hops}-hops/{measurement}_net-m-{hops}_stack-maggie-gm-{hops}-hops_action-1_{algo}" + \
        file_path

def get_vals(hops, measurement, algo, meas_type):
    return np.loadtxt(open(get_path(hops, measurement, algo, meas_type)), skiprows=1)

def get_quartile_set(x, constant=1.5):
    upper_quartile = np.percentile(x, 75)
    lower_quartile = np.percentile(x, 25)
    IQR = (upper_quartile - lower_quartile) * constant
    return (lower_quartile - IQR, upper_quartile + IQR)

def out_of_bounds_within(quartile_set, arr):
    for val in arr.tolist():
        if val >= quartile_set[0] and val <= quartile_set[1]:
            return False
    
    return True

def find_out_of_bounds_end(quartile_set, arr, start):
    for i, val in enumerate(arr[start:].tolist(), start):
        if val >= quartile_set[0] and val <= quartile_set[1]:
            return i
    
    return len(arr) - 1

def detect_outliers(quartile_set, arr, start, range=1200, grace=1000):
    outliers = []

    for i, val in enumerate(arr[start:].tolist(), start):
        if not (val >= quartile_set[0] and val <= quartile_set[1]) \
        and out_of_bounds_within(quartile_set, arr[i : (i + range)]):
            if i < grace:
                start = 0
            else:
                start = i - grace

            end = find_out_of_bounds_end(quartile_set, arr, i) + grace
            outliers += [(start, end)]
            outliers += detect_outliers(quartile_set, arr, end + 1)
            return outliers
    
    return outliers
    

def remove_intervals_from_list(list, outliers):
    new = []

    cont = 0
    for o in outliers:
        new += list[cont:o[0]]
        cont = o[1]

    new += list[cont:]
    return new 


def visualize(vals, qs, outliers):
    graph = sns.lineplot(data=vals, lw=0.5)
    graph.axhline(qs[0], color='r', lw=0.2)
    graph.axhline(qs[1], color='r', lw=0.2)

    for o in outliers:
        graph.axvspan(o[0], o[1], color='g', alpha=0.25)
    plt.show()

removed = []

for meas_type in types:
    print(f'Cleaning {meas_type} values..')

    # first iteration: find out how much we truncate
    # optionally: plot visualization (for fiddling with the parameters)
    for hop in hops[meas_type]:
        seq = hop['seq']
        for al in algos:
            vals = get_vals(hop['num'], seq, al['path'], meas_type)
            qs = get_quartile_set(vals)
            outliers = detect_outliers(qs, vals, 0)

            total = 0
            for o in outliers:
                total += o[1] - o[0]

            print(f"Removing {total} dp for {al['name']} / {hop['num']} hops")
            removed.append(total)
            seq += 1
            
            if DEBUG:
                visualize(vals, qs, outliers)

    truncation = 450000 - round(max(removed), -3)
    print(f"Min removed: {min(removed)}, Max removed: {max(removed)} | Truncation for {meas_type}: {truncation}")
    print(f'Storing cleaned data for {meas_type}. This might take a while..')
    # second iteration: write out cleaned, truncated data
    for hop in hops[meas_type]:
        seq = hop['seq']
        for al in algos:
            vals = get_vals(hop['num'], seq, al['path'], meas_type)
            qs = get_quartile_set(vals)
            outliers = detect_outliers(qs, vals, 0)

            # remove outliers
            vals = remove_intervals_from_list(vals.tolist(), outliers)
            # truncate
            vals = vals[0:truncation]

            if DEBUG:
                visualize(vals, qs, [])

            path = get_path_out(hop['num'], seq, al['path'], meas_type)
            os.makedirs(os.path.dirname(path), exist_ok=False)
            with open(path, 'w+') as f:
                np.savetxt(f, vals, delimiter="\n", header=f"Truncated to {truncation} values", fmt="%s")
            seq += 1