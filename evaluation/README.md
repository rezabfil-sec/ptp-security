## `/evaluation`
**Important note**: please adjust the `repo_path` variable in all three relevant python files before running them (`stats.py`, `plots.py`, `clean.py`). 

+ `data/`
  + `data/measurements-e2e/` contains the unaltered data and artifacts for E2E
  + `data/measurements-logSync-e2e/` contains the unaltered data and artifacts for the E2E logSync experiments
  + `data/measurements-logSync-tc/` contains the unaltered data and artifacts for the TC logSync experiments
  + `data/measurements-p2p/` contains the unaltered data and artifacts for P2P
  + `data/measurements-residence/` contains the unaltered data and artifacts for the residence measurements
  + `data/measurements-tc/` contains the unaltered data and artifacts for TC
+ `scripts/`
  + `scripts/clean.py` is the script used for cleaning the data from the measurements. **Use this before running `stats.py` or `plots.py`**
  + `scripts/stats.py` generates the table data (optionally: in TeX format) for mean, median, variance, stddev
  + `scripts/plots.py` generates all plots used in the paper. The plots are generated in the `pgf` format, but a `png` switch also exists
