#include <catch2/catch_session.hpp>
#include <sylvan_obj.hpp>
#include <simple_logger.h>
#include <thread>

int main( int argc, char* argv[] ) {
    simple_logger::Config::logFileName = "tests.log";
    lace_start(3, 0);
    sylvan::Sylvan::initPackage(1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
    sylvan::sylvan_init_zdd();
    // avoid Lace's race condition
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    lace_suspend();

    int result = Catch::Session().run(argc, argv);

    lace_resume();
    sylvan::Sylvan::quitPackage();
    // avoid Lace's race condition
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    lace_stop();
    return result;
}
