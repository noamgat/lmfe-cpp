#include <catch2/catch.hpp>
#include <lmfe/lmfe.hpp>
#include <string>
#include "./testutils.hpp"

const std::string SAMPLE_SCHEMA = R"({"$defs": {"InnerModel": {"properties": {"list_of_ints": {"items": {"type": "integer"}, "title": "List Of Ints", "type": "array"}}, "required": ["list_of_ints"], "title": "InnerModel", "type": "object"}, "IntegerEnum": {"enum": [1, 2, 3, 4], "title": "IntegerEnum", "type": "integer"}, "StringEnum": {"enum": ["One", "Two", "Three", "Four"], "title": "StringEnum", "type": "string"}}, "properties": {"num": {"title": "Num", "type": "integer"}, "dec": {"anyOf": [{"type": "number"}, {"type": "null"}], "default": null, "title": "Dec"}, "message": {"anyOf": [{"type": "string"}, {"type": "null"}], "default": null, "title": "Message"}, "list_of_strings": {"anyOf": [{"items": {"type": "string"}, "maxItems": 3, "minItems": 2, "type": "array"}, {"type": "null"}], "default": null, "title": "List Of Strings"}, "inner_dict": {"anyOf": [{"additionalProperties": {"$ref": "#/$defs/InnerModel"}, "type": "object"}, {"type": "null"}], "default": null, "title": "Inner Dict"}, "simple_dict": {"anyOf": [{"additionalProperties": {"type": "integer"}, "type": "object"}, {"type": "null"}], "default": null, "title": "Simple Dict"}, "list_of_models": {"anyOf": [{"items": {"$ref": "#/$defs/InnerModel"}, "type": "array"}, {"type": "null"}], "default": null, "title": "List Of Models"}, "enum": {"anyOf": [{"$ref": "#/$defs/IntegerEnum"}, {"type": "null"}], "default": null}, "enum_dict": {"anyOf": [{"additionalProperties": {"$ref": "#/$defs/StringEnum"}, "type": "object"}, {"type": "null"}], "default": null, "title": "Enum Dict"}, "true_or_false": {"anyOf": [{"type": "boolean"}, {"type": "null"}], "default": null, "title": "True Or False"}}, "required": ["num"], "title": "SampleModel", "type": "object"})";

TEST_CASE("Minimal test", "[json]")
{
    auto parser = CharacterLevelParserPtr(new JsonSchemaParser(SAMPLE_SCHEMA));
    assert_parser_with_string("{}", parser, false);
    assert_parser_with_string(R"({"num":1})", parser, true);
}
