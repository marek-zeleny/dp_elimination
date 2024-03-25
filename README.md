# DP Elimination

Program for preprocessing CNF formulas by eliminating some of its variables using the Davis-Putnam algorithm.
It uses Zero-suppressed Binary Decision Diagrams (ZDDs) for efficient representation and manipulation of CNF formulas.

## Prerequisites

- Linux-based system
- CMake (version 3.14 or later)
- C++ compiler supporting C++20 (GNU version 10 or later)

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
it contains a `build` directory with successfully compiled binaries using the steps above.

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

The `junk` directory is default destination for output files.
Feel free to adjust this behaviour to your needs in the config file.
The program outputs the following files (their names can be adjusted in the config/with options):
- `result.cnf` the resulting formula after eliminating variables (in DIMACS format)
- `metrics.json` metrics collected during the algorithm's execution, including eliminated variables, performance
    details, etc.
- `dp.log` runtime log (mostly for resolving issues)

The `data` directory also contains some more example input formulas to play around with.
