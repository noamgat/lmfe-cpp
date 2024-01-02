#pragma once
#include <vector>
#include <string>
#include "./characterlevelparser.hpp"
#include "./nlohmann_json.hpp"
#include "./valijson_nlohmann_bundled.hpp"

#include <memory>

using json = nlohmann::json;
using Schema = valijson::Schema;

class JsonSchemaParser;
extern CharacterLevelParserPtr get_parser(JsonSchemaParser* parser, const valijson::Subschema* schema);

class JsonSchemaParser : public CharacterLevelParser
{
public:
    JsonSchemaParser(const std::string &schema_string, CharacterLevelParserConfig *config);

    ~JsonSchemaParser() 
    {
        // std::cout << "JsonSchemaParser destructor called" << std::endl;
    }
    CharacterLevelParserPtr add_character(char new_character);

    std::string get_allowed_characters() const;

    virtual bool can_end() const;


public:
    struct _Context {
        Schema model_class;
        JsonSchemaParser* active_parser;
        std::string alphabet_without_quotes;
    };
    typedef std::shared_ptr<_Context> ContextPtr;

    std::vector<CharacterLevelParserPtr> object_stack;
    ContextPtr context;
    CharacterLevelParserConfig* config;
    int num_consecutive_whitespaces;
    std::string last_parsed_string;
    std::string last_non_whitespace_character;

protected:
     JsonSchemaParser(ContextPtr context, CharacterLevelParserConfig* config, const std::vector<CharacterLevelParserPtr>& updated_stack, int num_consecutive_whitespaces) 
     : context(context), config(config), object_stack(updated_stack), num_consecutive_whitespaces(num_consecutive_whitespaces) {

     }
};


