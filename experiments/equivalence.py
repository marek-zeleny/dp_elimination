#!/usr/bin/env python3

import sys
from pysat.formula import CNF
from pysat.solvers import Solver


def does_formula_imply(f1, f2):
    with Solver(bootstrap_with=f1) as solver:
        for clause in f2:
            assumptions = [-l for l in clause]
            sat = solver.solve(assumptions=assumptions)
            if sat:
                print(f"clause {clause} is not implied by formula")
                print(f1)
                return False
    return True


def compare_formulas_pysat(path1, path2):
    cnf1 = CNF(from_file=path1)
    cnf2 = CNF(from_file=path2)
    f1_impl_f2 = does_formula_imply(cnf1, cnf2)
    if not f1_impl_f2:
        print("formula 1 does not imply formula 2")
        return False
    f2_impl_f1 = does_formula_imply(cnf2, cnf1)
    if not f2_impl_f1:
        print("formula 2 does not imply formula 1")
        return False
    return True


if __name__ == '__main__':
    if (len(sys.argv)) != 3:
        print(f"usage: {sys.argv[0]} formula1 formula2", file=sys.stderr)
        sys.exit(2)

    path1 = sys.argv[1]
    path2 = sys.argv[2]

    equivalent = compare_formulas_pysat(path1, path2)

    if equivalent:
        print("equivalent")
    else:
        print("not equivalent")
        sys.exit(1)
