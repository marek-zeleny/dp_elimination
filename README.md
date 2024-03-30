# DP Elimination

Program for preprocessing CNF formulas by eliminating some of its variables using the Davis-Putnam algorithm.
It uses Zero-suppressed Binary Decision Diagrams (ZDDs) for efficient representation and manipulation of CNF formulas.

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
mkdir junk
build/dp --config data/config.toml --input-file data/example.cnf
```

The file `data/config.toml` contains reasonable default configuration.
All the values can be overwritten with command line options.
Run `build/dp --help` for more information about individual options.

The `junk/` directory is default destination for output files.
Feel free to adjust this behaviour to your needs in the config file.
The program outputs the following files (their names can be adjusted in the config/with options):
- `result.cnf` the resulting formula after eliminating variables (in DIMACS format)
- `metrics.json` metrics collected during the algorithm's execution, including eliminated variables, performance
    details, etc.
- `dp.log` runtime log (mostly for resolving issues)

The `data` directory also contains some more example input formulas to play around with.

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
the `dp` application, then run
```
experiments/experiments.py run build/dp
```
This script is likely to run a couple of minutes. After it's done, you will find results of the conducted experiments
at `experiments/results`.
Feel free to investigate the `result.cnf` formula, or the collected `metrics.json` manually.
However, the script also generate some summary tables and plots from the collected metrics; for default behavior run
```
experiments/experiments.py summarize results/
experiments/experiments.py visualize results/
```
and inspect the created `.png` and `.md` files.

For further options and customization, see output from
```
experiments/experiments.py --help
```
