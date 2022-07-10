import numpy as np

# ------------------ ADJUST ME ------------------
repo_path = "/path/to/repo/ptp-security"
readable = True # set to False to print in TeX table format
meas_type = 'e2e' # select from e2e, p2p, tc
# -----------------------------------------------

file_path = "/node-6_abe/phc_cmp_processed.out"

avg_removed = 0
round_to = 1

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

algos = [   
    {'path': 'nosec', 'name': 'No Security'},
    {'path': 'hmacsha512256', 'name': 'HMAC-SHA-512-256'},
    {'path': 'blake2b', 'name': 'Blake2b'},
    {'path': 'blake3', 'name': 'Blake3b'},
    {'path': 'dummyr1ac', 'name': 'Dummy'},
]

def get_path_clean(hops, measurement, algo):
    return f"{repo_path}/evaluation/data/measurements-{meas_type}-cleaned/" + \
        f"{hops}-hops/{measurement}_net-m-{hops}_stack-maggie-gm-{hops}-hops_action-1_{algo}" + \
        file_path

def get_vals(hops, measurement, algo):
    return np.loadtxt(open(get_path_clean(hops, measurement, algo)), skiprows=1)

if readable:
    for hop in hops[meas_type]:
        print('=============================================================================================================')
        print(f'Statistics for {hop["num"]} hops')
        print('=============================================================================================================')
        for al in algos:
            print(f"{al['name']} --- ")
            vals = get_vals(hop['num'], hop['seq'], al['path'])
            hop['seq'] += 1
            print(f"Mean {np.round(np.mean(vals), round_to)} | Median {np.median(vals)} | " + \
                f"Variance {np.round(np.var(vals), round_to)} | Std. {np.round(np.std(vals), round_to)}\n")
        
        print('=============================================================================================================\n\n')
else:
    print(f'Tables for {meas_type}')
    print("% Mean ---")
    for i, al in enumerate(algos):
        print(f"{al['name']} & ", end='')
        for hop in hops[meas_type]:
            vals = get_vals(hop['num'], hop['seq'] + i, al['path'])
            avg = np.round(np.average(vals), round_to)

            if (hop['num'] != 9):
                print(f"{avg} & ", end='')
            else:
                print(f"{avg}", end='')
        print(" \\\\")

    print("% Median ---")
    for i, al in enumerate(algos):
        print(f"{al['name']} & ", end='')
        for hop in hops[meas_type]:
            vals = get_vals(hop['num'], hop['seq'] + i, al['path'])
            med = int(np.median(vals))
            if (hop['num'] != 9):
                print(f"{med} & ", end='')
            else:
                print(f"{med}", end='')
        print(" \\\\")

    print("% Variance ---")
    for i, al in enumerate(algos):
        print(f"{al['name']} & ", end='')
        for hop in hops[meas_type]:
            vals = get_vals(hop['num'], hop['seq'] + i, al['path'])
            var = np.round(np.var(vals), round_to)
            if (hop['num'] != 9):
                print(f"{var} & ", end='')
            else:
                print(f"{var}", end='')
        print(" \\\\")

    print("% Stddev ---")
    for i, al in enumerate(algos):
        print(f"{al['name']} & ", end='')
        for hop in hops[meas_type]:
            vals = get_vals(hop['num'], hop['seq'] + i, al['path'])
            std = np.round(np.std(vals), round_to)
            if (hop['num'] != 9):
                print(f"{std} & ", end='')
            else:
                print(f"{std}", end='')
        print(" \\\\")