#pragma once

#include <lmfe/lmfe.hpp>
#include <stdexcept>
#include <string>

class CharacterNotAllowedException : public std::exception {
public:
    explicit CharacterNotAllowedException(const std::string& message) : message_(message) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

void assert_parser_with_string_direct(const std::string& string, CharacterLevelParserPtr parser, bool expect_success) {
    for (size_t idx = 0; idx < string.size(); ++idx) {
        char character;
        try
        {
            char character = string[idx];
            if (parser->get_allowed_characters().find(character) != std::string::npos) {
                parser = parser->add_character(character);
            } else {
                if (expect_success) {
                    throw CharacterNotAllowedException("Parser does not allow '" + std::string(1, character) + "' at index " + std::to_string(idx));
                } else {
                    return;  // Success
                }
            }
        }
        catch (const CharacterNotAllowedException &e)
        {
            throw;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error parsing '" + std::string(1, character) + "' at index " + std::to_string(idx) + ": " + e.what());
        }
    }
    if (parser->can_end() && !expect_success) {
        throw std::runtime_error("Parser succeeded when it should have failed");
    }
    if (!parser->can_end() && expect_success) {
        throw std::runtime_error("Parser did not reach end state when it should have");
    }
}


void assert_parser_with_string(const std::string& string, CharacterLevelParserPtr parser, bool expect_success) {
    assert_parser_with_string_direct(string, parser, expect_success);
}
