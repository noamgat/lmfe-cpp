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

void assert_parser_with_string(const std::string &string, CharacterLevelParserPtr parser, bool expect_success);
