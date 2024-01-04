#include "lmfe/tokenizerdata.hpp"

TokenizerPrefixTree::TokenizerPrefixTree(std::vector<std::tuple<int, std::string, bool>> regular_tokens) {
    root = new TokenizerPrefixTreeNode();
    for (const auto& token : regular_tokens) {
        int token_idx;
        std::string decoded;
        bool is_new_word;
        std::tie(token_idx, decoded, is_new_word) = token;
        tokens_to_strs[token_idx] = decoded;
        _add_token_to_tree(decoded, token_idx, root);
        if (is_new_word) {
            new_word_tokens.insert(token_idx);
        }
    }
}

void TokenizerPrefixTree::_add_token_to_tree(const std::string& token_str, int token_idx, TokenizerPrefixTreeNode* node) {
    for (char character : token_str) {
        if (node->children.find(character) == node->children.end()) {
            node->children[character] = new TokenizerPrefixTreeNode();
        }
        node = node->children[character];
    }
    node->tokens.push_back(token_idx);
}

void TokenEnforcerTokenizerData::initialize()
{
    regular_tokens = get_regular_tokens();
    eos_token_id = get_eos_token_id();
    
    tokenizer_tree = new TokenizerPrefixTree(regular_tokens);
    for (const auto& token_str : tokenizer_tree->root->children) {
        tokenizer_alphabet += token_str.first;
    }
}