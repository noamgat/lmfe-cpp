#include <llama.h>
#include <lmfe/tokenizerdata.hpp>

class LlamaCppTokenizerData : public TokenEnforcerTokenizerData {
public:
    LlamaCppTokenizerData(llama_model* model) : model(model) {}
    // Methods that children have to implement
    virtual std::string decode(const std::vector<int>& tokens) const {
        return decode_tokens(tokens);
    };

protected:
    virtual std::vector<std::tuple<int, std::string, bool>> get_regular_tokens() const {
        auto vocab_size = llama_n_vocab(model);
        auto regular_tokens = std::vector<std::tuple<int, std::string, bool>>();
        std::vector<int> special_token_ids;

        for (int i = 0; i < vocab_size; ++i)
        {
            bool is_regular_token = llama_token_get_type(model, i) == llama_token_type::LLAMA_TOKEN_TYPE_NORMAL;
            if (!is_regular_token) {
                continue;
            }
            auto token_str = decode_token(i);
            auto token_sequence_str = decode_tokens({i});
            bool is_new_word = token_str.size() > token_sequence_str.size();
            regular_tokens.push_back(std::make_tuple(i, token_str, is_new_word));
        }
        return regular_tokens;
    };
    virtual int get_eos_token_id() const {
        return llama_token_eos(model);
    };

    // Taken from llamacpp/common/common.cpp llama_token_to_piece

    std::string decode_token(int token) const {
        std::vector<char> result(8, 0);
        const int n_tokens = llama_token_to_piece(model, token, result.data(), result.size());
        if (n_tokens < 0) {
            result.resize(-n_tokens);
            int check = llama_token_to_piece(model, token, result.data(), result.size());
            GGML_ASSERT(check == -n_tokens);
        } else {
            result.resize(n_tokens);
        }

        return std::string(result.data(), result.size());
    }

    std::string decode_tokens(const std::vector<int> & tokens) const {
        const llama_token bos_id = llama_token_bos(model);

        std::string piece;
        std::string result;

        for (size_t i = 0; i < tokens.size(); ++i) {
            piece = decode_token(tokens[i]);

            // remove the leading space of the first non-BOS token
            if (((tokens[0] == bos_id && i == 1) || (tokens[0] != bos_id && i == 0)) && piece[0] == ' ') {
                piece = piece.substr(1);
            }

            result += piece;
        }

        return result;
    }

    llama_model* model;
};
 