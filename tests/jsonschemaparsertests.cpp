#include <catch2/catch.hpp>
#include <lmfe/lmfe.hpp>

TEST_CASE( "Basic Json Schema Parsing Test", "[json]" ) {
    auto parser = CharacterLevelParserPtr(new JsonSchemaParser("{}"));
}
