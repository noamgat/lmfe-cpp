#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <stdexcept>
#include <memory>
#include <iostream>

class CharacterLevelParser;
typedef std::shared_ptr<CharacterLevelParser> CharacterLevelParserPtr;


class CharacterLevelParser : public std::enable_shared_from_this<CharacterLevelParser> {
public:
    virtual ~CharacterLevelParser() 
    {
        std::cout << "CharacterLevelParser::~CharacterLevelParser()" << std::endl;
    }

    virtual CharacterLevelParserPtr add_character(char new_character) = 0;
    virtual std::string get_allowed_characters() const = 0;
    virtual bool can_end() const = 0;
    virtual std::string shortcut_key() const { return ""; }
    virtual std::size_t cache_key() const { return 0; }
};

class CharacterLevelParserConfig {
public:
    std::string alphabet;
};

class StringParser : public CharacterLevelParser {
public:
    StringParser(const std::string& string) : target_str(string) {}

    CharacterLevelParserPtr add_character(char new_character) override {
        if (target_str.find(new_character) == 0) {
            return std::make_shared<StringParser>(target_str.substr(1));
        } else {
            throw std::invalid_argument("Expected '" + target_str.substr(0, 1) + "' but got '" + new_character + "'");
        }
    }

    std::string get_allowed_characters() const override {
        return target_str.empty() ? "" : target_str.substr(0, 1);
    }

    bool can_end() const override {
        return target_str.empty();
    }

private:
    std::string target_str;
};

class ForceStopParser : public CharacterLevelParser {
public:
    CharacterLevelParserPtr add_character(char new_character) override {
        return CharacterLevelParserPtr(this);
    }

    std::string get_allowed_characters() const override {
        return "";
    }

    bool can_end() const override {
        return true;
    }
};

class UnionParser : public CharacterLevelParser {
public:
    UnionParser(const std::vector<CharacterLevelParserPtr>& parsers) : parsers(parsers) {}

    CharacterLevelParserPtr add_character(const char new_character) override {
        std::vector<CharacterLevelParserPtr> relevant_parsers;
        for (CharacterLevelParserPtr parser : parsers) {
            if (parser->get_allowed_characters().find(new_character) != std::string::npos) {
                relevant_parsers.push_back(parser->add_character(new_character));
            }
        }
        if (relevant_parsers.size() == 1) {
            return CharacterLevelParserPtr(relevant_parsers[0]);
        }
        return CharacterLevelParserPtr(new UnionParser(relevant_parsers));
    }

    std::string get_allowed_characters() const override {
        std::unordered_set<char> allowed;
        for (CharacterLevelParserPtr parser : parsers) {
            const std::string& allowed_characters = parser->get_allowed_characters();
            allowed.insert(allowed_characters.begin(), allowed_characters.end());
        }
        return std::string(allowed.begin(), allowed.end());
    }

    bool can_end() const override {
        for (CharacterLevelParserPtr parser : parsers) {
            if (parser->can_end()) {
                return true;
            }
        }
        return false;
    }

private:
    std::vector<CharacterLevelParserPtr> parsers;
};

class SequenceParser : public CharacterLevelParser {
public:
    SequenceParser(const std::vector<CharacterLevelParserPtr>& parsers) : parsers(parsers) {}

    CharacterLevelParserPtr add_character(char new_character) override {
        std::vector<CharacterLevelParserPtr> legal_parsers;
        for (std::size_t idx = 0; idx < parsers.size(); ++idx) {
            CharacterLevelParserPtr parser = parsers[idx];
            if (parser->get_allowed_characters().find(new_character) != std::string::npos) {
                CharacterLevelParserPtr updated_parser = parser->add_character(new_character);
                std::vector<CharacterLevelParserPtr> next_parsers(parsers.begin() + idx + 1, parsers.end());
                next_parsers.insert(next_parsers.begin(), updated_parser);
                legal_parsers.push_back(CharacterLevelParserPtr(new SequenceParser(next_parsers)));
            }
            if (!parser->can_end()) {
                break;
            }
        }
        if (legal_parsers.size() == 1) {
            return legal_parsers[0];
        }
        return CharacterLevelParserPtr(new UnionParser(legal_parsers));
    }

    std::string get_allowed_characters() const override {
        std::unordered_set<char> allowed_characters;
        for (CharacterLevelParserPtr parser : parsers) {
            const std::string& parser_allowed_characters = parser->get_allowed_characters();
            allowed_characters.insert(parser_allowed_characters.begin(), parser_allowed_characters.end());
            if (!parser->can_end()) {
                break;
            }
        }
        return std::string(allowed_characters.begin(), allowed_characters.end());
    }

    bool can_end() const override {
        for (CharacterLevelParserPtr parser : parsers) {
            if (!parser->can_end()) {
                return false;
            }
        }
        return true;
    }

private:
    std::vector<CharacterLevelParserPtr> parsers;
};

