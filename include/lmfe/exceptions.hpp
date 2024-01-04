#pragma once
#include <exception>
#include <string>

class LMFormatEnforcerException : public std::exception {
public:
    explicit LMFormatEnforcerException(const std::string& message) : message_(message) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};