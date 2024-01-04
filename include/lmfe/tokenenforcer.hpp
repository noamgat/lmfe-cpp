#pragma once

#include <vector>
#include <set>
#include "./characterlevelparser.hpp"
#include "./tokenizerdata.hpp"
#include "./exceptions.hpp"

//https://stackoverflow.com/a/53283994/1075114
struct VectorHasher {
    int operator()(const std::vector<int> &V) const {
        int hash = V.size();
        for(auto &i : V) {
            hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

typedef const std::vector<int> FrozenTokenVector;

class TokenEnforcer
{
public:
    struct OutputTensorState {
        CharacterLevelParserPtr parser;
        std::vector<int> allowed_tokens;
        std::vector<int> current_word_tokens;
    };

    TokenEnforcer(TokenEnforcerTokenizerData* tokenizer_data, CharacterLevelParserPtr parser): tokenizer_data(tokenizer_data), root_parser(parser) {
        
    }

    FrozenTokenVector& get_allowed_tokens(FrozenTokenVector token_sequence) {
        FrozenTokenVector sent_tuple(token_sequence.begin(), token_sequence.end());
        FrozenTokenVector prev_step_tuple(token_sequence.begin(), token_sequence.end() - 1);

        if (prefix_states.count(sent_tuple) > 0) {
            return prefix_states[sent_tuple]->allowed_tokens;
        } else if (prefix_states.count(prev_step_tuple) == 0) {
            OutputTensorState* state = new OutputTensorState();
            state->parser = root_parser;
            prefix_states[sent_tuple] = state;
            _compute_allowed_tokens(sent_tuple, state);
            return state->allowed_tokens;
        } else {
            OutputTensorState* prev_step_state = prefix_states[prev_step_tuple];
            OutputTensorState* new_state = _apply_new_characters(prev_step_state, token_sequence);
            prefix_states[sent_tuple] = new_state;
            _compute_allowed_tokens(sent_tuple, new_state);
            return new_state->allowed_tokens;
        }
    }

private:
    std::unordered_map<FrozenTokenVector, OutputTensorState*, VectorHasher> prefix_states;
    CharacterLevelParserPtr root_parser;
    TokenEnforcerTokenizerData* tokenizer_data;
    // Other member variables

    TokenEnforcer::OutputTensorState* _apply_new_characters(OutputTensorState* state, FrozenTokenVector& token_sequence) {
        TokenEnforcer::OutputTensorState* new_state = new TokenEnforcer::OutputTensorState();
        new_state->parser = state->parser;
        TokenizerPrefixTree* tokenizer_tree = tokenizer_data->tokenizer_tree;
        int new_token = token_sequence.back();
        std::string new_characters;
        if (std::find(tokenizer_tree->new_word_tokens.begin(), tokenizer_tree->new_word_tokens.end(), new_token) != tokenizer_tree->new_word_tokens.end())
        {
            new_state->current_word_tokens = {new_token};
            new_characters = tokenizer_tree->tokens_to_strs[new_token];
        }
        else
        {
            new_state->current_word_tokens = state->current_word_tokens;
            new_state->current_word_tokens.push_back(new_token);
            std::string prev_decoded = tokenizer_data->decode(state->current_word_tokens);
            std::string new_decoded = tokenizer_data->decode(new_state->current_word_tokens);
            new_characters = new_decoded.substr(prev_decoded.length());
        }
        for (char character : new_characters) {
            auto allowed_characters = new_state->parser->get_allowed_characters();
            if (std::find(allowed_characters.begin(), allowed_characters.end(), character) != allowed_characters.end())
            {
                new_state->parser = new_state->parser->add_character(character);
            }
            else
            {
                // This can happen in beam / batch scenarios, when some of the batches finished but others are continuing.
                //logging.debug("Received an invalid character '" + character + "', switching to ForceStopParser");
                new_state->parser = CharacterLevelParserPtr(new ForceStopParser());
                break;
            }
        }
        return new_state;
    }

    void _collect_allowed_tokens(CharacterLevelParserPtr parser, TokenizerPrefixTreeNode* tree_node, std::vector<int>& allowed_tokens) {
        allowed_tokens.insert(allowed_tokens.end(), tree_node->tokens.begin(), tree_node->tokens.end());
        std::string allowed_characters = parser->get_allowed_characters();
        std::set<char> allowed_characters_set(allowed_characters.begin(), allowed_characters.end());

        
        std::string characters_to_explore;
        for (const auto& entry : tree_node->children) {
            if (allowed_characters_set.find(entry.first) != allowed_characters_set.end()) {
                characters_to_explore += entry.first;
            }
        }
        
        /*
        if (shortcut_key.has_value() && std::get<0>(shortcut_key.value()) == "json_freetext") {
            assert(std::get<shortcut_key.value().size() == 4);
            std::tuple<std::string, int, int, int> shortcut = shortcut_key.value();
            int cur_len = std::get<1>(shortcut);
            int min_len = std::get<2>(shortcut);
            int max_len = std::get<3>(shortcut);
            auto cache = this->tokenizer_tree.json_freetext_tokens;

            int min_remaining = std::min(cache.max_token_len, std::max(0, min_len - cur_len));
            int max_allowed_len = std::min(cache.max_token_len, max_len - cur_len);

            std::vector<int> allowed_tokens_cache = cache.lookup_allowed_tokens(min_remaining, max_allowed_len);
            allowed_tokens.insert(allowed_tokens.end(), allowed_tokens_cache.begin(), allowed_tokens_cache.end());
            characters_to_explore = std::set<char>{'"'}.intersection(characters_to_explore);
        }
        */

        for (char character : characters_to_explore) {
            CharacterLevelParserPtr next_parser = parser->add_character(character);
            TokenizerPrefixTreeNode* next_tree_node = tree_node->children[character];
            _collect_allowed_tokens(next_parser, next_tree_node, allowed_tokens);
        }
    }

    void _compute_allowed_tokens(FrozenTokenVector& state_tokens, TokenEnforcer::OutputTensorState* state) {
        try {
            std::vector<int> allowed_tokens;
            /*
            auto cache_key = state.parser.cache_key();
            if (cache_key != nullptr && allowed_token_cache.count(cache_key) > 0) {
                state.allowed_tokens = allowed_token_cache[cache_key];
                return;
            }
            auto shortcut_key = state.parser.shortcut_key();
            */
            _collect_allowed_tokens(state->parser, tokenizer_data->tokenizer_tree->root, allowed_tokens/*, shortcut_key*/);
            if (state->parser->can_end()) {
                allowed_tokens.push_back(tokenizer_data->eos_token_id);
            }
            if (allowed_tokens.empty()) {
                throw std::runtime_error("Parser reached state with no allowed tokens");
            }
            state->allowed_tokens = allowed_tokens;
            /*
            if (cache_key != nullptr) {
                allowed_token_cache[cache_key] = allowed_tokens;
            }
            */
        } catch (const LMFormatEnforcerException& ex) {
            // Getting an LMFormatEnforcerException means that we know what the user did wrong,
            // and we can give a nice error message for them to fix.
            throw;
        } catch (const std::exception& ex) {
            // Other exceptions are potential bugs and should be reported
            std::string prefix = tokenizer_data->decode(state_tokens);
            std::cerr << "Unknown LMFormatEnforcer Problem. Prefix: '" << prefix << "'" << std::endl
                      << "Terminating the parser. Please open an issue at" << std::endl
                      << "https://github.com/noamgat/lm-format-enforcer/issues with the prefix and "
                      << "CharacterLevelParser parameters" << std::endl;
            state->allowed_tokens = { tokenizer_data->eos_token_id };
        }
    }
};
