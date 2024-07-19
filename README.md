# DP Elimination

Program for preprocessing CNF formulas by eliminating some of its variables using the Davis-Putnam algorithm.
It uses Zero-suppressed Binary Decision Diagrams (ZDDs) for efficient representation and manipulation of CNF formulas.

The program was developed as part of a Master thesis at Faculty of Mathematics and Physics, Charles University in Prague.
The thesis is attached as `thesis.pdf` and includes detailed user and programming documentation.

## Prerequisites

- Linux-based system
- CMake (version 3.14 or later)
- C++ compiler supporting C++20 (GNU version 10 or later)
- Python (for running experiments)

## Compilation

```
git clone --recurse-submodules -j6 <remote_repository>
cd dp_elimination
mkdir build
cd build
cmake ..
cmake --build . -j6
```

In the rest of these sections we assume that you are in the root directory of the project (`dp_elimination`) and that
it contains a `build/` directory with successfully compiled binaries using the steps above.

## Running tests

```
build/tests
```

## Running application

```
build/dp --config experiments/default_config.toml --heuristic minimal_bloat example_formula.cnf
```

The input formula needs to be in the DIMACS CNF format.
Example input formulas can be found in the `experiments/inputs/` directory.
The file `experiments/default_config.toml` contains reasonable default configuration.
We recommend using it in combination with `experiments/setups/all_minimizations.toml`.
All the values can be overwritten with command line options.
Run `build/dp --help` for more information about individual options.

The program outputs the following files (their names can be adjusted in the config or with options):
- `result.cnf` the resulting formula (in DIMACS CNF format)
- `metrics.json` metrics collected during the algorithm's execution, including eliminated variables, performance
    details, etc.
- `dp.log` runtime log (mostly for resolving issues)

## Running experiments

The `experiments/` directory contains experimental setups and a Python script for automated execution of those.
If you have the [Conda package manager](https://docs.conda.io/en/latest/) available on your system, you can create
a Conda environment from the included `environment.yml` file:
```
conda env create -f experiments/environment.yml
```
and activate the environment before running the Python script:
```
conda activate dp-experiments
```

If you can't use Conda, you can install the necessary packages manually - they're listed under `dependencies` in the
environment file.

To run the predefined experiments with all default settings, make sure that you have already [compiled](#compilation)
the `dp` application.
It might also be necessary to increase the program stack limit to avoid segmentation fault in some experimens:
```
ulimit -s 16777216
```
Then run
```
experiments/experiments.py run build/dp
```
Note that this script will run a lot of experiments by default.
We recommend adjusting it beforehand.
After it's done, you will find results of the conducted experiments
at `experiments/results/`.
Feel free to investigate the `result.cnf` formula, or the collected `metrics.json` manually.
However, the script also generate some summary tables and plots from the collected metrics; for default behavior run
```
experiments/experiments.py setup-summary results/
experiments/experiments.py visualize-setup-summary results/summary_data.csv
```
or to investigate individual runs rather than the whole experiment:
```
experiments/experiments.py summarize results/
experiments/experiments.py visualize results/
```
and inspect the created `.png`, `.md`, and `.csv` files.

For further options and customization, see output from
```
experiments/experiments.py --help
```
