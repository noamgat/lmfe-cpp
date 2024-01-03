#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <string>

struct TokenizerPrefixTreeNode;

struct TokenizerPrefixTreeNode
{
    std::vector<int> tokens;
    std::unordered_map<char, TokenizerPrefixTreeNode*> children;
};

class TokenizerPrefixTree {
public:
    TokenizerPrefixTreeNode* root;
    std::unordered_set<int> new_word_tokens;
    std::unordered_map<int, std::string> tokens_to_strs;

    TokenizerPrefixTree(std::vector<std::tuple<int, std::string, bool>> regular_tokens);

private:
    void _add_token_to_tree(const std::string& token_str, int token_idx, TokenizerPrefixTreeNode* node);
};

class TokenEnforcerTokenizerData
{
public:
    void initialize();

    std::vector<std::tuple<int, std::string, bool>> regular_tokens;
    TokenizerPrefixTree* tokenizer_tree;
    std::function<std::string(const std::vector<int>&)> decoder;
    int eos_token_id;
    std::string tokenizer_alphabet;

    // Methods that children have to implement
    virtual std::string decode(const std::vector<int>& tokens) const = 0;

protected:
    virtual std::vector<std::tuple<int, std::string, bool>> get_regular_tokens() const = 0;
    virtual int get_eos_token_id() const = 0;

    ~TokenEnforcerTokenizerData();
};
