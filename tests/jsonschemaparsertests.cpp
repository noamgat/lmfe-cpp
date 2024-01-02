#include <catch2/catch.hpp>
#include <string>

#include "./testutils.hpp"
#include <lmfe/lmfe.hpp>
#include <lmfe/nlohmann_json.hpp>

using json = nlohmann::json;

const std::string SAMPLE_SCHEMA = R"({"$defs": {"InnerModel": {"properties": {"list_of_ints": {"items": {"type": "integer"}, "title": "List Of Ints", "type": "array"}}, "required": ["list_of_ints"], "title": "InnerModel", "type": "object"}, "IntegerEnum": {"enum": [1, 2, 3, 4], "title": "IntegerEnum", "type": "integer"}, "StringEnum": {"enum": ["One", "Two", "Three", "Four"], "title": "StringEnum", "type": "string"}}, "properties": {"num": {"title": "Num", "type": "integer"}, "dec": {"anyOf": [{"type": "number"}, {"type": "null"}], "default": null, "title": "Dec"}, "message": {"anyOf": [{"type": "string"}, {"type": "null"}], "default": null, "title": "Message"}, "list_of_strings": {"anyOf": [{"items": {"type": "string"}, "maxItems": 3, "minItems": 2, "type": "array"}, {"type": "null"}], "default": null, "title": "List Of Strings"}, "inner_dict": {"anyOf": [{"additionalProperties": {"$ref": "#/$defs/InnerModel"}, "type": "object"}, {"type": "null"}], "default": null, "title": "Inner Dict"}, "simple_dict": {"anyOf": [{"additionalProperties": {"type": "integer"}, "type": "object"}, {"type": "null"}], "default": null, "title": "Simple Dict"}, "list_of_models": {"anyOf": [{"items": {"$ref": "#/$defs/InnerModel"}, "type": "array"}, {"type": "null"}], "default": null, "title": "List Of Models"}, "enum": {"anyOf": [{"$ref": "#/$defs/IntegerEnum"}, {"type": "null"}], "default": null}, "enum_dict": {"anyOf": [{"additionalProperties": {"$ref": "#/$defs/StringEnum"}, "type": "object"}, {"type": "null"}], "default": null, "title": "Enum Dict"}, "true_or_false": {"anyOf": [{"type": "boolean"}, {"type": "null"}], "default": null, "title": "True Or False"}}, "required": ["num"], "title": "SampleModel", "type": "object"})";

void test_json_schema_parsing_with_string(const std::string& string, const std::string& schema_str, bool expect_success) {
    auto parser = std::make_shared<JsonSchemaParser>(schema_str, nullptr);
    assert_parser_with_string(string, parser, expect_success);
    if (expect_success) {
        // If expecting success, also check minified and pretty-printed
        json string_json = json::parse(string);
        auto minified = string_json.dump();
        assert_parser_with_string(minified, parser, expect_success);
        auto pretty_printed = string_json.dump(2);
        assert_parser_with_string(pretty_printed, parser, expect_success);
    }
}

TEST_CASE("test_minimal", "[json]")
{
    test_json_schema_parsing_with_string(R"({"num":1})", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_required_field", "[json]")
{
    test_json_schema_parsing_with_string("{}", SAMPLE_SCHEMA, false);
    test_json_schema_parsing_with_string(R"({"dec": 1.1})", SAMPLE_SCHEMA, false);
}
