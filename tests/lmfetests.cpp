#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <lmfe/lmfe.hpp>

TEST_CASE( "Basic String Parser Check", "[main]" ) {
    auto parser = CharacterLevelParserPtr(new StringParser("abc"));
    REQUIRE( parser->get_allowed_characters() == "a" );
    parser = parser->add_character('a');
    REQUIRE( parser->get_allowed_characters() == "b" );
    parser = parser->add_character('b');
    REQUIRE( parser->get_allowed_characters() == "c" );
}
