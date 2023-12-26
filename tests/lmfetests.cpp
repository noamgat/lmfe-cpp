#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <lmfe/lmfe.hpp>

TEST_CASE( "Quick check", "[main]" ) {
    auto added = add_numbers(1, 3);

    REQUIRE( added == 4 );
}
