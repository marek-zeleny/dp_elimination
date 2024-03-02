#include <vector>
#include <sylvan_obj.hpp>
#include "args_parser.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"
#include "algorithms/dp_elimination.hpp"

TASK_2(int, impl, int, argc, char**, argv)
{
    using namespace dp;
    // parse args
    ArgsParser args = ArgsParser::parse(argc, argv);
    if (!args.success()) {
        return 1;
    }

    // initialize Sylvan
    Sylvan::initPackage(1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
    sylvan_init_zdd();

    // load input file
    SylvanZddCnf cnf = SylvanZddCnf::from_file(args.get_input_cnf_file_name());
    std::cout << "Input formula has " << cnf.clauses_count() << " clauses" << std::endl;

    // perform the algorithm
    size_t num_vars = args.get_eliminated_vars();
    std::cout << "Eliminating " << num_vars << " variables..." << std::endl;
    SylvanZddCnf result = eliminate_vars(cnf, num_vars);

    // write result to file
    std::string file_name = "data/result.cnf";
    result.write_dimacs_to_file(file_name);
    std::cout << "Formula with " << result.clauses_count() << " clauses written to file " << file_name << std::endl;

    // quit sylvan, free memory
    Sylvan::quitPackage();
    return 0;
}

int main(int argc, char *argv[])
{
    // initialize Lace
    int n_workers = 0; // automatically detect number of workers
    size_t deque_size = 0; // default value for the size of task deques for the workers
    lace_start(n_workers, deque_size);
    // run main Lace thread
    return RUN(impl, argc, argv);

    // The lace_start command also exits Lace after _main is completed.
}
