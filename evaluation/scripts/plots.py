from matplotlib.axes import Axes
import numpy as np
import seaborn as sns
import matplotlib as mpl
import matplotlib.pyplot as plt

# ------------------ ADJUST ME ------------------
repo_path = "/path/to/repo/ptp-security"
generate_png = True # set to True to output the plots as png instead of pgf
# -----------------------------------------------

meas_path = "/node-6_abe/phc_cmp_processed.out"
residence_path = "/node-5_otto/residence_processed"

round_to = 1
n_bins = 60000

hops = {
    'e2e': [
        {'num': 4, 'seq': 0},
        {'num': 5, 'seq': 5},
        {'num': 6, 'seq': 0},
        {'num': 7, 'seq': 4},
        {'num': 8, 'seq': 9},
        {'num': 9, 'seq': 14},
    ], 'p2p': [
        {'num': 4, 'seq': 0},
        {'num': 5, 'seq': 5},
        {'num': 6, 'seq': 10},
        {'num': 7, 'seq': 15},
        {'num': 8, 'seq': 20},
        {'num': 9, 'seq': 25},
    ], 'tc': [
        {'num': 4, 'seq': 0},
        {'num': 5, 'seq': 1},
        {'num': 6, 'seq': 0},
        {'num': 7, 'seq': 4},
        {'num': 8, 'seq': 9},
        {'num': 9, 'seq': 14},
    ], 'residence': [
        {'num': 2, 'seq': 0, 'xlim_fact': (7, 10)}
    ]
}

types = ['e2e', 'p2p', 'tc']

algos = [   
    {'path': 'nosec', 'name': 'No Security'},
    {'path': 'hmacsha512256', 'name': 'HMAC-SHA-512-256'},
    {'path': 'blake2b', 'name': 'Blake2b'},
    {'path': 'blake3', 'name': 'Blake3b'},
    {'path': 'dummyr1ac', 'name': 'Dummy'},
]

rel_path = repo_path + "/evaluation/data/reliability/"
rel_vals_lisa = np.loadtxt(open(rel_path + 'lisa_reliability_processed.out'))
rel_vals_abe = np.loadtxt(open(rel_path + 'abe_reliability_processed.out'))

def get_path_unclean(hops, measurement, algo, meas_type, file_path):
    return f"{repo_path}/evaluation/data/measurements-{meas_type}/" + \
        f"{hops}-hops/{measurement}_net-m-{hops}_stack-maggie-gm-{hops}-hops_action-1_{algo}" + \
        file_path

def get_path_clean(hops, measurement, algo, meas_type, file_path):
    return f"{repo_path}/evaluation/data/measurements-{meas_type}-cleaned/" + \
        f"{hops}-hops/{measurement}_net-m-{hops}_stack-maggie-gm-{hops}-hops_action-1_{algo}" + \
        file_path

def get_residence(hops, measurement, algo):
    return np.loadtxt(open(get_path_unclean(hops, measurement, algo, 'residence', residence_path))) / 1000 # adjusted to us

def get_vals(hops, measurement, algo, meas_type):
    return np.loadtxt(open(get_path_unclean(hops, measurement, algo, meas_type, meas_path)), skiprows=1)

def get_vals_clean(hops, measurement, algo, meas_type):
    return np.loadtxt(open(get_path_clean(hops, measurement, algo, meas_type, meas_path)), skiprows=1)

def set_plt_config(sz=(4,4)):
        plt.figure(figsize=sz)
        # plt.style.use('seaborn')
        plt.rc('text', usetex=True)
        plt.rc('text.latex', preamble=r'\usepackage{amsmath}')
        plt.rcParams.update({
                "font.family": "serif",
                "font.size": 12,
                "text.usetex": True,
                "pgf.rcfonts": False
        })

# from https://stackoverflow.com/a/52921726
def fix_hist_step_vertical_line_at_end(ax):
    axpolygons = [poly for poly in ax.get_children(
    ) if isinstance(poly, mpl.patches.Polygon)]
    for poly in axpolygons:
        poly.set_xy(poly.get_xy()[:-1])

# from https://jwalton.info/Matplotlib-latex-PGF/
def set_size(width_pt, fraction=1, subplots=(1, 1)):
    """Set figure dimensions to sit nicely in our document.

    Parameters
    ----------
    width_pt: float
            Document width in points
    fraction: float, optional
            Fraction of the width which you wish the figure to occupy
    subplots: array-like, optional
            The number of rows and columns of subplots.
    Returns
    -------
    fig_dim: tuple
            Dimensions of figure in inches
    """
    # Width of figure (in pts)
    fig_width_pt = width_pt * fraction
    # Convert from pt to inches
    inches_per_pt = 1 / 72.27

    # Golden ratio to set aesthetic figure height
    golden_ratio = (5**.5 - 1) / 2

    # Figure width in inches
    fig_width_in = fig_width_pt * inches_per_pt
    # Figure height in inches
    fig_height_in = fig_width_in * golden_ratio * (subplots[0] / subplots[1])

    return (fig_width_in, fig_height_in)

if not generate_png:
    mpl.use('pgf')

# ---------------------------------------------------------------------------------------------------------------------------
# Reliability line plot pic:plot:reliability
# ---------------------------------------------------------------------------------------------------------------------------
set_plt_config(sz=(8,2))
ax = sns.lineplot()
abe, = ax.plot(rel_vals_abe, label='abe', linewidth=0.35)
lisa, = ax.plot(rel_vals_lisa, label='lisa', linewidth=0.35)
ax.legend(handles=[abe, lisa], loc="lower right", prop={'size': 12})
ax.set_xlabel('Data point index')
ax.set_ylabel('Offset (ns)')
ax.grid(True)

if generate_png:
    plt.savefig(f'./reliability.png', bbox_inches = "tight")
else:
    plt.savefig(f'./reliability.pgf', format='pgf', bbox_inches = "tight")

plt.clf()
# ---------------------------------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------------------------------
# Line plot for stddev pic:plot:line-stddev-{meas-type}
# ---------------------------------------------------------------------------------------------------------------------------
for meas_type in types:
    x = []
    std_devs = []
    label = []

    for al in algos:
        label.append(al['name'])

    for i, al in enumerate(algos):
        al['std'] = []
        for hop in hops[meas_type]:
            vals = get_vals_clean(hop['num'], hop['seq'] + i, al['path'], meas_type)
            std = np.round(np.std(vals), round_to)
            al['std'].append(std)

        std_devs.append(al['std'])

    set_plt_config(sz=(5, 3))
    ax = sns.lineplot(data=std_devs, linewidth=0.9, markers=True)
    ax.legend(label, prop={'size': 11})
    ax.set_xticks([0,1,2,3,4,5])
    ax.set_xticklabels([4,5,6,7,8,9])
    ax.set_xlabel('Number of hops')
    ax.set_ylabel('Standard Deviation (ns)')
    ax.grid(True)

    if generate_png:
        plt.savefig(f'./line-stddev-{meas_type}.png', bbox_inches = "tight")
    else:
        plt.savefig(f'./line-stddev-{meas_type}.pgf', format='pgf', bbox_inches = "tight")
        
plt.clf()
# ---------------------------------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------------------------------
# Residence CDF plot for pic:plot:residence
# ---------------------------------------------------------------------------------------------------------------------------
sz = set_size(300, 1.75)
set_plt_config(sz=sz)
fig, ax = plt.subplots(figsize=sz)

hop = hops['residence'][0]
for i, alg in enumerate(algos):
    ax.hist(get_residence(hop['num'], hop['seq'] + i, alg['path']), 45000, density=True, histtype='step', cumulative=True, label=alg['name'], lw=1.2)

fix_hist_step_vertical_line_at_end(ax)
ax.grid(True)
ax.legend(loc="upper left")
ax.set_xlabel(f'Residence Time (us)')
ax.set_ylabel('CDF')

ax.set_ylim(0, 1.05)
ax.set_xlim(0, 340)
if generate_png:
    plt.savefig('./residence-cdf.png')
else:
    plt.savefig('./residence-cdf.pgf',
                format='pgf', bbox_inches="tight")
plt.clf()
# ---------------------------------------------------------------------------------------------------------------------------