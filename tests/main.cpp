#include <catch2/catch_session.hpp>
#include <sylvan_obj.hpp>

TASK_2(int, tests, int, argc, char**, argv) {
    sylvan::Sylvan::initPackage(1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
    sylvan::sylvan_init_zdd();

    int result = Catch::Session().run(argc, argv);

    sylvan::Sylvan::quitPackage();
    return result;
}

int main( int argc, char* argv[] ) {
    lace_start(0, 0);
    return RUN(tests, argc, argv);
}
