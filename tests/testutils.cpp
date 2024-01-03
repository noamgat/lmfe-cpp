#include "./testutils.hpp"
#include <llama.h>
#include "./llamacpp_adapter.hpp"

const char* MODEL_PATH = "phi2.gguf";

llama_model *model = nullptr;
LlamaCppTokenizerData* tokenizer_data = nullptr;

void initialize_llama_if_needed() {
    if (model == nullptr) {
        llama_backend_init(false);
        llama_model_params model_params = llama_model_default_params();
        model = llama_load_model_from_file(MODEL_PATH, model_params);
        tokenizer_data = new LlamaCppTokenizerData(model);
        tokenizer_data->initialize();
    }
}

// Taken from llamacpp/common/common.cpp
std::vector<llama_token> _llama_tokenize(
    const struct llama_model * model,
           const std::string & text,
                        bool   add_bos,
                        bool   special = false) {
    // upper limit for the number of tokens
    int n_tokens = text.length() + add_bos;
    std::vector<llama_token> result(n_tokens);
    n_tokens = llama_tokenize(model, text.data(), text.length(), result.data(), result.size(), add_bos, special);
    if (n_tokens < 0) {
        result.resize(-n_tokens);
        int check = llama_tokenize(model, text.data(), text.length(), result.data(), result.size(), add_bos, special);
        GGML_ASSERT(check == -n_tokens);
    } else {
        result.resize(n_tokens);
    }
    return result;
}

void assert_parser_with_string_token_enforcer(const std::string &string, CharacterLevelParserPtr parser, bool expect_success)
{
    initialize_llama_if_needed();

    std::vector<llama_token> tokens_list = _llama_tokenize(model, string, true);
    
}

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
    assert_parser_with_string_token_enforcer(string, parser, expect_success);
}