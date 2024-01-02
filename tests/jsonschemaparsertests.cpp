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
    test_json_schema_parsing_with_string(R"(
        {"num":1}
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_parsing_test_model", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"num":1,"dec":1.1,"message":"ok","list_of_strings":["a","b","c"],"inner_dict":{"a":{"list_of_ints":[1,2,3]}}}
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_invalid_key_in_json_string", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"numa":1,"dec":1.1,"message":"ok","list_of_strings":["a","b","c"],"inner_dict":{"a":{"list_of_ints":[1,2,3]}}}
    )", SAMPLE_SCHEMA, false);
}

TEST_CASE("test_incomplete_json", "[json]")
{
    // Intentionally missing closing }
    test_json_schema_parsing_with_string(R"(
        {"num":1,"dec":1.1,"message":"ok","list_of_strings":["a","b","c"],"inner_dict":{"a":{"list_of_ints":[1,2,3]}}
    )", SAMPLE_SCHEMA, false);
}

TEST_CASE("test_invalid_value_type_in_json_string", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"num:"1","dec":1.1,"message":"ok","list_of_strings":["a","b","c"],"inner_dict":{"a":{"list_of_ints":[1,2,3]}}}
    )", SAMPLE_SCHEMA, false);
}

TEST_CASE("test_list_of_objects", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"list_of_models":[{"list_of_ints":[1,2,3]},{"list_of_ints":[4,5,6]}],"num":1}
    )", SAMPLE_SCHEMA, true);
    test_json_schema_parsing_with_string(R"(
        {"list_of_models": [{"list_of_ints":[1, 2, 3]} , {"list_of_ints":[4,5,6]}],"num":1}
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_simple_dict", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"simple_dict":{"a":1,"b":2,"c":3},"num":1}
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_int_enum", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"enum":4,"num":1}
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_invalid_int_enum_value", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"enum":5,"num":1}
    )", SAMPLE_SCHEMA, false);
}

TEST_CASE("test_str_enum", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"enum_dict":{"a":"One","b":"Two","c":"Three","d":"Four"},"num":1}
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_invalid_str_enum_value", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"enum_dict":{"a":"Onee"},"num":1}
    )", SAMPLE_SCHEMA, false);
}

TEST_CASE("test_whitespaces", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        { "message": "","num":1}
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_whitespace_before_number", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"num": 1, "dec": 1.1}
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_whitespace_before_close", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"num":1 }
    )", SAMPLE_SCHEMA, true);
}

TEST_CASE("test_required_field", "[json]")
{
    test_json_schema_parsing_with_string("{}", SAMPLE_SCHEMA, false);
    test_json_schema_parsing_with_string(R"({"dec": 1.1})", SAMPLE_SCHEMA, false);
}

TEST_CASE("test_boolean_field", "[json]")
{
    test_json_schema_parsing_with_string(R"(
        {"num":1,"true_or_false":false}
    )", SAMPLE_SCHEMA, true);
    test_json_schema_parsing_with_string(R"(
        {"num":1,"true_or_false":true}
    )", SAMPLE_SCHEMA, true);
    test_json_schema_parsing_with_string(R"(
        {"num":1,"true_or_false": true}
    )", SAMPLE_SCHEMA, true);
    test_json_schema_parsing_with_string(R"(
        {"num":1,"true_or_false":falsy}
    )", SAMPLE_SCHEMA, false);
}

TEST_CASE("test_unspecified_dict", "[json]")
{
    std::string schema = R"(
        {"properties": {"num": {"title": "Num", "type": "integer"}, "d": {"title": "D", "type": "object"}}, "required": ["num", "d"], "title": "DictModel", "type": "object"}
    )";
    test_json_schema_parsing_with_string(R"(
        {"num":1,"d":{"k":"v"}}
    )", schema , true);
}

TEST_CASE("test_unspecified_list", "[json]")
{
    std::string schema = R"(
        {"properties": {"num": {"title": "Num", "type": "integer"}, "l": {"items": {}, "title": "L", "type": "array"}}, "required": ["num", "l"], "title": "DictModel", "type": "object"}
    )";
    test_json_schema_parsing_with_string(R"(
        {"num":1,"l":[1,2,3,"b"]}
    )", schema, true);
}

TEST_CASE("test_list_length_limitations", "[json]")
{
    std::string no_strings = R"(
        {"num":1,"list_of_strings":[]}
    )";
    test_json_schema_parsing_with_string(no_strings, SAMPLE_SCHEMA, false);
    std::string one_string = R"(
        {"num":1,"list_of_strings":["a"]}
    )";
    test_json_schema_parsing_with_string(one_string, SAMPLE_SCHEMA, false);
    std::string two_strings = R"(
        {"num":1,"list_of_strings":["a", "b"]}
    )";
    test_json_schema_parsing_with_string(two_strings, SAMPLE_SCHEMA, true);
    std::string three_strings = R"(
        {"num":1,"list_of_strings":["a","b","c"]}
    )";
    test_json_schema_parsing_with_string(three_strings, SAMPLE_SCHEMA, true);
    std::string four_strings = R"(
        {"num":1,"list_of_strings":["a","b","c","d"]}
    )";
    test_json_schema_parsing_with_string(four_strings, SAMPLE_SCHEMA, false);


    std::string empty_list_ok_schema = R"(
        {"properties": {"num": {"title": "Num", "type": "integer"}, "list_of_strings": {"anyOf": [{"items": {"type": "string"}, "maxItems": 1, "minItems": 0, "type": "array"}, {"type": "null"}], "default": null, "title": "List Of Strings"}}, "required": ["num"], "title": "EmptyListOKModel", "type": "object"}
    )";
    test_json_schema_parsing_with_string(no_strings, empty_list_ok_schema, true);
    test_json_schema_parsing_with_string(one_string, empty_list_ok_schema, true);
    test_json_schema_parsing_with_string(two_strings, empty_list_ok_schema, false);

    std::string list_of_exactly_one_schema = R"(
        {"properties": {"num": {"title": "Num", "type": "integer"}, "list_of_strings": {"anyOf": [{"items": {"type": "string"}, "maxItems": 1, "minItems": 1, "type": "array"}, {"type": "null"}], "default": null, "title": "List Of Strings"}}, "required": ["num"], "title": "ListOfExactlyOneModel", "type": "object"}
    )";
    test_json_schema_parsing_with_string(no_strings, list_of_exactly_one_schema, false);
    test_json_schema_parsing_with_string(one_string, list_of_exactly_one_schema, true);
    test_json_schema_parsing_with_string(two_strings, list_of_exactly_one_schema, false);

    std::string list_of_no_min_length_schema = R"(
        {"properties": {"num": {"title": "Num", "type": "integer"}, "list_of_strings": {"anyOf": [{"items": {"type": "string"}, "maxItems": 1, "type": "array"}, {"type": "null"}], "default": null, "title": "List Of Strings"}}, "required": ["num"], "title": "ListOfNoMinLengthModel", "type": "object"}
    )";
    test_json_schema_parsing_with_string(no_strings, list_of_no_min_length_schema, true);
    test_json_schema_parsing_with_string(one_string, list_of_no_min_length_schema, true);
    test_json_schema_parsing_with_string(two_strings, list_of_no_min_length_schema, false);
}

/*
def test_string_escaping():
    for escaping_character in BACKSLASH_ESCAPING_CHARACTERS:
        test_string = f'{{"num":1,"message":"hello {BACKSLASH}{escaping_character} world"}}'
        _test_json_schema_parsing_with_string(test_string, SampleModel.schema(), True)
    for non_escaping_character in 'a1?':
        test_string = f'{{"num":1,"message":"hello {BACKSLASH}{non_escaping_character} world"}}'
        _test_json_schema_parsing_with_string(test_string, SampleModel.schema(), False)

    # Unicode
    test_string = f'{{"num":1,"message":"hello {BACKSLASH}uf9f0 world"}}'
    _test_json_schema_parsing_with_string(test_string, SampleModel.schema(), True)

    # Not enough unicode digits
    test_string = f'{{"num":1,"message":"hello {BACKSLASH}uf9f world"}}'
    _test_json_schema_parsing_with_string(test_string, SampleModel.schema(), False)

    # Unicode digit outside of hex range
    test_string = f'{{"num":1,"message":"hello {BACKSLASH}uf9fP world"}}'
    _test_json_schema_parsing_with_string(test_string, SampleModel.schema(), False)
*/

/*
TEST_CASE("test_comma_after_all_object_keys_fails", "[json]")
{
    std::string schema = R"(
        {"properties": {"key": {"title": "Key", "type": "string"}}, "required": ["key"], "title": "SomeSchema", "type": "object"}
    )";
    REQUIRE_THROWS_MATCHES(test_json_schema_parsing_with_string(R"(
        {"key": "val",
    )", schema, true), std::runtime_error, Catch::Matchers::Contains("Error parsing ',' at index"));
}

TEST_CASE("test_single_quote_must_not_be_escaped", "[json]")
{
    std::string schema = R"(
        {"properties": {"key": {"title": "Key", "type": "string"}}, "required": ["key"], "title": "SomeSchema", "type": "object"}
    )";
    REQUIRE_THROWS_MATCHES(test_json_schema_parsing_with_string(R"(
        {"key": "I\'m a string"}
    )", schema, true), std::runtime_error, Catch::Matchers::Contains("Error parsing ''' at index"));
}
*/

TEST_CASE("test_string_length_limitation", "[json]")
{
    std::string schema = R"(
        {"properties": {"key": {"maxLength": 3, "minLength": 2, "title": "Key", "type": "string"}}, "required": ["key"], "title": "SomeSchema", "type": "object"}
    )";

    std::string str_base = R"(
        {{"key": "HERE"}}
    )";
    for (int str_length = 0; str_length < 10; ++str_length) {
        std::string str = str_base;
        str.replace(str.find("HERE"), std::string("HERE").size(), std::string(str_length, '.'));
        bool expect_success = str_length >= 2 && str_length <= 3;
        test_json_schema_parsing_with_string(str, schema, expect_success);
    }
}

TEST_CASE("test_any_json_object", "[json]")
{
    std::string any_schema = "";
    test_json_schema_parsing_with_string(R"(
        {}
    )", any_schema, true);
    test_json_schema_parsing_with_string(R"(
        {"a": 1, "b": 2.2, "c": "c", "d": [1,2,3, null], "e": {"ee": 2}}
    )", any_schema, true);
    test_json_schema_parsing_with_string(R"(
        true
    )", any_schema, true);
    test_json_schema_parsing_with_string(R"(
        "str"
    )", any_schema, true);
}

/*
def test_long_json_object():
    from urllib.request import urlopen
    import json
    json_url = 'https://microsoftedge.github.io/Demos/json-dummy-data/64KB.json'
    json_text = urlopen(json_url).read().decode('utf-8')
    # These are several "hacks" on top of the json file in order to bypass some shortcomings of the unit testing method.
    json_text = ''.join(c for c in json_text if 0 < ord(c) < 127)
    json_text = json_text.replace('.",', '",')
    json_text = json_text.replace(' ",', '",')
    json_text = json_text.replace('.",', '",')
    json_text = json.dumps(json.loads(json_text)[:20])

    profile_file_path = None  # '64KB.prof' 
    _test_json_schema_parsing_with_string(json_text, None, True, profile_file_path=profile_file_path)
*/

TEST_CASE("test_union", "[json]")
{
    std::string schema = R"(
        {"properties": {"key": {"anyOf": [{"type": "integer"}, {"type": "string"}], "title": "Key"}}, "required": ["key"], "title": "SchemaWithUnion", "type": "object"}
    )";
    test_json_schema_parsing_with_string(R"(
        {"key": 1}
    )", schema, true);
    test_json_schema_parsing_with_string(R"(
        {"key": "a"}
    )", schema, true);
    test_json_schema_parsing_with_string(R"(
        {"key": 1.2}
    )", schema, false);
    test_json_schema_parsing_with_string(R"(
        {"key": false}
    )", schema, false);
}