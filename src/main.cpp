#include <cassert>
#include <cstdio>
#include <vector>

// only needed for sylvan_stats_report(), otherwise everything should be defined
// in sylvan_obj.hpp
#include <sylvan.h>
#include <sylvan_obj.hpp>

#include "args_parser.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"
#include "algorithms/dp_elimination.hpp"
#include "algorithms/unit_propagation.hpp"

using namespace sylvan;

VOID_TASK_0(simple_bdd_test)
{
    Bdd one = Bdd::bddOne(); // the True terminal
    Bdd zero = Bdd::bddZero(); // the False terminal

    // check if they really are the True/False terminal
    assert(one.GetBDD() == sylvan_true);
    assert(zero.GetBDD() == sylvan_false);

    Bdd a = Bdd::bddVar(0); // create a BDD variable x_0
    Bdd b = Bdd::bddVar(1); // create a BDD variable x_1

    // check if a really is the Boolean formula "x_0"
    assert(!a.isConstant());
    assert(a.TopVar() == 0);
    assert(a.Then() == one);
    assert(a.Else() == zero);

    // check if b really is the Boolean formula "x_1"
    assert(!b.isConstant());
    assert(b.TopVar() == 1);
    assert(b.Then() == one);
    assert(b.Else() == zero);

    // compute !a
    Bdd not_a = !a;

    // check if !!a is really a
    assert((!not_a) == a);

    // compute a * b and !(!a + !b) and check if they are equivalent
    Bdd a_and_b = a * b;
    Bdd not_not_a_or_not_b = !(!a + !b);
    assert(a_and_b == not_not_a_or_not_b);

    // perform some simple quantification and check the results
    Bdd ex = a_and_b.ExistAbstract(a); // \exists a . a * b
    assert(ex == b);
    Bdd andabs = a.AndAbstract(b, a); // \exists a . a * b using AndAbstract
    assert(ex == andabs);
    Bdd univ = a_and_b.UnivAbstract(a); // \forall a . a * b
    assert(univ == zero);

    // alternative method to get the cube "ab" using bddCube
    BddSet variables = a * b;
    std::vector<unsigned char> vec = {1, 1};
    assert(a_and_b == Bdd::bddCube(variables, vec));

    // test the bddCube method for all combinations
    assert((!a * !b) == Bdd::bddCube(variables, std::vector<uint8_t>({0, 0})));
    assert((!a * b)  == Bdd::bddCube(variables, std::vector<uint8_t>({0, 1})));
    assert((!a)      == Bdd::bddCube(variables, std::vector<uint8_t>({0, 2})));
    assert((a * !b)  == Bdd::bddCube(variables, std::vector<uint8_t>({1, 0})));
    assert((a * b)   == Bdd::bddCube(variables, std::vector<uint8_t>({1, 1})));
    assert((a)       == Bdd::bddCube(variables, std::vector<uint8_t>({1, 2})));
    assert((!b)      == Bdd::bddCube(variables, std::vector<uint8_t>({2, 0})));
    assert((b)       == Bdd::bddCube(variables, std::vector<uint8_t>({2, 1})));
    assert(one       == Bdd::bddCube(variables, std::vector<uint8_t>({2, 2})));
}

VOID_TASK_0(zdd_cnf_test)
{
    using namespace dp;
    std::vector<std::vector<int32_t>> clauses {
        {1, 2, 3},
        {2, 4},
        {1, 3, 4},
        {-5},
        {2, 5, 6},
        //{},
    };
    SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
    cnf.draw_to_file("zdd.gv");
    cnf.print_clauses();
    std::cout << std::endl;
    SylvanZddCnf cnf_2 = cnf.subset1(2);
    cnf_2.print_clauses();
    std::cout << std::endl;
    SylvanZddCnf cnf_no_2 = cnf.subset0(2);
    cnf_no_2.print_clauses();
}

VOID_TASK_0(zdd_experiments)
{
    using namespace dp;
    std::vector<std::vector<int32_t>> clauses {
        {1, -1, 3},
        {1, -1, 3, 4},
        {-4, -6},
        //{-4, 5},
        {2, -4, 5, -6},
        {-2, -5, 6},
        {-5},
    };
    SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
    cnf.print_clauses();
    cnf.draw_to_file("cnf.gv");
    ZDD zdd = zdd_eval(cnf.get_zdd(), 10, 1);
    SylvanZddCnf cnf2 = SylvanZddCnf(zdd);
    std::cout << "evaluate variable 5 to True:" << std::endl;
    cnf2.print_clauses();
    SylvanZddCnf cnf3 = cnf.remove_tautologies();
    std::cout << "remove tautologies:" << std::endl;
    cnf3.print_clauses();
    SylvanZddCnf cnf4 = cnf.remove_subsumed_clauses();
    std::cout << "remove subsumed clauses:" << std::endl;
    cnf4.print_clauses();

    std::vector<std::vector<int32_t>> clauses2 {
        {-1, 2},
        {-2, 3},
        {-1, 3},
    };
    std::vector<std::vector<int32_t>> no_absorbed = remove_absorbed_clauses(clauses2);
    SylvanZddCnf cnf5 = SylvanZddCnf::from_vector(no_absorbed);
    std::cout << "remove absorbed clauses:" << std::endl;
    cnf5.print_clauses();

    std::vector<std::vector<int32_t>> no_absorbed2 = remove_absorbed_clauses(clauses);
    SylvanZddCnf cnf6 = SylvanZddCnf::from_vector(no_absorbed2);
    std::cout << "remove absorbed clauses:" << std::endl;
    cnf6.print_clauses();
}

VOID_TASK_0(dp_elimination)
{
    using namespace dp;
    std::vector<std::vector<int32_t>> clauses {
        {1, 2, 3},
        {2, 4},
        {1, 3, 4},
        {2, 5, 6},
        {-4},
    };
    SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
    cnf.print_clauses();
    std::cout << std::endl;
    SylvanZddCnf cnf_4 = eliminate(cnf, 4);
    cnf_4.print_clauses();
    std::cout << std::endl;
    bool result = is_sat(cnf);
    std::cout << result << std::endl << std::endl;

    std::vector<std::vector<int32_t>> clauses2 {
        {1, 2, 3},
        {-2, 4},
        {-1, 3, 4},
        {2, 5, 6},
        {-4},
        {-3},
    };
    SylvanZddCnf cnf2 = SylvanZddCnf::from_vector(clauses2);
    cnf2.print_clauses();
    std::cout << std::endl;
    bool result2 = is_sat(cnf2);
    std::cout << result2 << std::endl;
    cnf2.draw_to_file("zdd.gv");
}

VOID_TASK_0(run_from_lace)
{
    // Initialize Sylvan
    // With starting size of the nodes table 1 << 21, and maximum size 1 << 27.
    // With starting size of the cache table 1 << 20, and maximum size 1 << 20.
    // Memory usage: 24 bytes per node, and 36 bytes per cache bucket
    // - 1<<24 nodes: 384 MB
    // - 1<<25 nodes: 768 MB
    // - 1<<26 nodes: 1536 MB
    // - 1<<27 nodes: 3072 MB
    // - 1<<24 cache: 576 MB
    // - 1<<25 cache: 1152 MB
    // - 1<<26 cache: 2304 MB
    // - 1<<27 cache: 4608 MB
    Sylvan::initPackage(1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);

    // Initialize the BDD module with granularity 1 (cache every operation)
    // A higher granularity (e.g. 6) often results in better performance in practice
    Sylvan::initBdd();
    sylvan_init_zdd();

    // Now we can do some simple stuff using the C++ objects.
    //CALL(simple_bdd_test);
    //CALL(zdd_cnf_test);
    CALL(zdd_experiments);
    //CALL(dp_elimination);

    // Report statistics (if SYLVAN_STATS is 1 in the configuration)
    //sylvan_stats_report(stdout);

    // And quit, freeing memory
    Sylvan::quitPackage();
}

TASK_2(int, impl, int, argc, char**, argv)
{
    using namespace dp;
    // parse args
    ArgsParser args = ArgsParser::parse(argc, argv);
    if (!args.success()) {
        return 1;
    }
    // initialize sylvan
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
    std::cout << "Formula with " << result.clauses_count() << " clauses written to file " << file_name << std::endl;
    result.write_dimacs_to_file(file_name);
    // quit sylvan, free memory
    Sylvan::quitPackage();
    return 0;
}

int main(int, char **)
//int main(int argc, char *argv[])
{
    // Initialize Lace
    int n_workers = 0; // automatically detect number of workers
    size_t deque_size = 0; // default value for the size of task deques for the workers

    lace_start(n_workers, deque_size);

    RUN(run_from_lace);
    //return RUN(impl, argc, argv);

    // The lace_start command also exits Lace after _main is completed.
}
