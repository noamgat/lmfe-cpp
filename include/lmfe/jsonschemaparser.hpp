#pragma once
#include <vector>
#include <string>
#include "./characterlevelparser.hpp"
#include "../nlohmann/json.hpp"
#include <memory>

using json = nlohmann::json;

class JsonSchemaParser : public CharacterLevelParser
{
public:
    JsonSchemaParser(const std::string& schema_string) {
        context = std::make_shared<_Context>();
        context->model_class = json::parse(schema_string);
        context->active_parser = this;
        context->alphabet_without_quotes = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        num_consecutive_whitespaces = 0;
        last_parsed_string = "";
        last_non_whitespace_character = "";
    }

    virtual CharacterLevelParserPtr add_character(char new_character) {
        return CharacterLevelParserPtr(this);
    }
    
    virtual std::string get_allowed_characters() const {
        return "";
    }
    virtual bool can_end() const {
        return true;
    }

protected:
    struct _Context {
        json model_class;
        JsonSchemaParser* active_parser;
        std::string alphabet_without_quotes;
    };

    std::vector<CharacterLevelParser*> object_stack;
    std::shared_ptr<_Context> context;
    int num_consecutive_whitespaces;
    std::string last_parsed_string;
    std::string last_non_whitespace_character;
};