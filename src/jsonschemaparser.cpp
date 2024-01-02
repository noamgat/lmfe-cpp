#include "lmfe/jsonschemaparser.hpp"

using namespace valijson::constraints;
using namespace valijson;

typedef const Subschema* JsonSchemaPtr;

JsonSchemaPtr ANY_JSON_OBJECT_SCHEMA = nullptr;
const std::string WHITESPACE_CHARACTERS = " \t\n\r\f\v";
const std::string COMPLETE_ALPHABET = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()_+-=[]{};:,./<>? `'\"";
const int MAX_CONSECUTIVE_WHITESPACES = 12;

template <class T>
const T *findConstraint(JsonSchemaPtr schema)
{
    for (size_t i=0; i<schema->m_constraints.size(); i++) {
        if (dynamic_cast<const T*>(schema->m_constraints[i].get())) {
            return dynamic_cast<const T*>(schema->m_constraints[i].get());
        }
    }
    return nullptr;
}

class BaseParsingState : public CharacterLevelParser
{
public:
    BaseParsingState(JsonSchemaParser* root): root(root) {}
protected:
    JsonSchemaParser *root;
};

enum class ObjectParsingStage {
    START_OBJECT,
    PARSING_KEY_OR_END,
    PARSING_KEY_VALUE_SEPARATOR,
    PARSING_VALUE,
    PARSING_SEPARATOR_OR_END,
    END_OBJECT
};

class PrimitiveParsingState : public BaseParsingState {
public:
    PrimitiveParsingState(JsonSchemaParser* root) : BaseParsingState(root) {
        parsed_string = "";
    }

    virtual PrimitiveParsingState* clone() const = 0;
   

    virtual CharacterLevelParserPtr add_character(char new_character) {
        PrimitiveParsingState* new_state = clone();
        new_state->parsed_string += new_character;
        return CharacterLevelParserPtr(new_state);
    }

    bool can_end() const override {
        return true;
    }

public:
    std::string parsed_string;
};


class StringParsingState : public PrimitiveParsingState {
private:
    std::vector<std::string> allowed_strings;
    bool seen_closing_quote;
    bool seen_opening_quote;
    size_t min_length;
    size_t max_length;
    bool require_closing_quote;
    bool require_opening_quote;

public:
    StringParsingState(
        JsonSchemaParser* root,
        std::vector<std::string> allowed_strings,
        bool require_opening_quote,
        bool require_closing_quote = true,
        size_t min_length = -1,
        size_t max_length = -1
    ) : PrimitiveParsingState(root),
        allowed_strings(allowed_strings),
        seen_closing_quote(false),
        seen_opening_quote(!require_opening_quote),
        require_closing_quote(require_closing_quote),
        require_opening_quote(require_opening_quote),
        min_length(min_length),
        max_length(max_length) {}

    virtual StringParsingState* clone() const {
        StringParsingState* clone = new StringParsingState(
            root,
            allowed_strings,
            require_opening_quote,
            require_closing_quote,
            min_length,
            max_length
        );
        clone->parsed_string = parsed_string;
        clone->seen_closing_quote = seen_closing_quote;
        clone->seen_opening_quote = seen_opening_quote;
        return clone;
    }

    virtual CharacterLevelParserPtr add_character(char new_character) {
        if ((parsed_string.empty() || seen_closing_quote) && WHITESPACE_CHARACTERS.find(new_character) != std::string::npos) {
            return shared_from_this();
        }
        CharacterLevelParserPtr newState = PrimitiveParsingState::add_character(new_character);
        StringParsingState* newStringState = static_cast<StringParsingState*>(newState.get());

        if (new_character == '"') {
            if (!seen_opening_quote) {
                newStringState->seen_opening_quote = true;
                newStringState->parsed_string = "";
            } else {
                newStringState->seen_closing_quote = true;
                newStringState->parsed_string = newStringState->parsed_string.substr(0, newStringState->parsed_string.size() - 1);
            }
        }
        if (new_character == '\\') {
            // Handle escaping characters
            // ...
            //# After a backslack we immediately have the escaping character, and if its 'u', we have 4 hex digits
            //escaping_character_parsers: List[CharacterLevelParser] = [StringParser(c) for c in BACKSLASH_ESCAPING_CHARACTERS]
            //hex_digit_parser: CharacterLevelParser = UnionParser([StringParser(c) for c in "0123456789abcdefABCDEF"])
            //unicode_components: List[CharacterLevelParser] = list([StringParser("u")] + [hex_digit_parser] * 4)
            //unicode_escape_parser: CharacterLevelParser = SequenceParser(unicode_components)
            //json_escaping_parser = UnionParser(escaping_character_parsers + [unicode_escape_parser])
            //self.root.context.active_parser.object_stack.append(json_escaping_parser)
        }
        return newState;
    }

    std::string get_allowed_characters() const {
        if (!seen_opening_quote) {
            return "\"" + WHITESPACE_CHARACTERS;
        }
        if (seen_closing_quote) {
            return WHITESPACE_CHARACTERS;
        }
        if (!allowed_strings.empty()) {
            std::vector<std::string> allowed_continuations;
            for (const std::string& s : allowed_strings) {
                if (s.find(parsed_string) == 0) {
                    allowed_continuations.push_back(s.substr(parsed_string.size()));
                }
            }
            std::vector<char> allowed_next_characters;
            for (const std::string& continuation : allowed_continuations) {
                if (!continuation.empty()) {
                    allowed_next_characters.push_back(continuation[0]);
                }
            }
            std::sort(allowed_next_characters.begin(), allowed_next_characters.end());
            allowed_next_characters.erase(std::unique(allowed_next_characters.begin(), allowed_next_characters.end()), allowed_next_characters.end());
            if (std::find(allowed_strings.begin(), allowed_strings.end(), parsed_string) != allowed_strings.end() && require_closing_quote) {
                allowed_next_characters.push_back('"');
            }
            if (parsed_string.empty() && (!seen_opening_quote || !require_opening_quote)) {
                allowed_next_characters.insert(allowed_next_characters.end(), WHITESPACE_CHARACTERS.begin(), WHITESPACE_CHARACTERS.end());
            }
            std::string allowed_characters(allowed_next_characters.begin(), allowed_next_characters.end());
            return allowed_characters;
        } else {
            if (min_length != -1 && parsed_string.size() < min_length) {
                return root->context->alphabet_without_quotes + "\\";
            }
            if (max_length != -1 && parsed_string.size() >= max_length) {
                return "\"";
            }
            //return root->config.alphabet + "\\";
            return COMPLETE_ALPHABET + WHITESPACE_CHARACTERS + "\\";
        }
    }

    bool can_end() const {
        if (require_closing_quote) {
            return seen_closing_quote;
        } else {
            if (!allowed_strings.empty()) {
                return std::find(allowed_strings.begin(), allowed_strings.end(), parsed_string) != allowed_strings.end();
            } else {
                return !parsed_string.empty();
            }
        }
    }
};

class NumberParsingState : public PrimitiveParsingState {
private:
    bool allow_floating_point;
    bool seen_decimal_point;
    bool seen_whitespace_after_digits;

public:
    NumberParsingState(JsonSchemaParser* root, bool allow_floating_point)
        : PrimitiveParsingState(root),
          allow_floating_point(allow_floating_point),
          seen_decimal_point(false),
          seen_whitespace_after_digits(false) {}

    NumberParsingState* clone() const override {
        NumberParsingState* clone = new NumberParsingState(root, allow_floating_point);
        clone->parsed_string = parsed_string;
        clone->seen_decimal_point = seen_decimal_point;
        clone->seen_whitespace_after_digits = seen_whitespace_after_digits;
        return clone;
    }

    CharacterLevelParserPtr add_character(char new_character) override {
        if (parsed_string.empty() && WHITESPACE_CHARACTERS.find(new_character) != std::string::npos) {
            return shared_from_this();
        }
        CharacterLevelParserPtr newState = PrimitiveParsingState::add_character(new_character);
        NumberParsingState* newNumberState = static_cast<NumberParsingState*>(newState.get());
        if (WHITESPACE_CHARACTERS.find(new_character) != std::string::npos) {
            if (!parsed_string.empty()) {
                newNumberState->seen_whitespace_after_digits = true;
            }
            return newState;
        }
        if (new_character == '.') {
            newNumberState->seen_decimal_point = true;
        }
        return newState;
    }

    std::string get_allowed_characters() const override {
        if (seen_whitespace_after_digits) {
            return WHITESPACE_CHARACTERS;
        }
        std::string allowed_characters = "0123456789";
        if (parsed_string.empty()) {
            allowed_characters += "-" + WHITESPACE_CHARACTERS;
        }
        if (allow_floating_point && !seen_decimal_point) {
            allowed_characters += ".";
        }
        if (!parsed_string.empty() && isdigit(parsed_string.back())) {
            allowed_characters += WHITESPACE_CHARACTERS;
        }
        return allowed_characters;
    }

    bool can_end() const override {
        return !parsed_string.empty() && (isdigit(parsed_string.back()) || seen_whitespace_after_digits);
    }
};

class ObjectParsingState : public BaseParsingState
{
public:
    JsonSchemaPtr schema_object;
    ObjectParsingStage current_stage;
    std::vector<std::string> existing_keys;
    std::string current_key;
    bool is_dictionary;

    ObjectParsingState(JsonSchemaPtr schema_object, JsonSchemaParser* root) :
        BaseParsingState(root),
        schema_object(schema_object),
        current_stage(ObjectParsingStage::START_OBJECT) {
        const PropertiesConstraint* propertiesConstraint = findConstraint<PropertiesConstraint>(schema_object);
        is_dictionary = (propertiesConstraint->m_properties.size() + propertiesConstraint->m_patternProperties.size()) == 0;
    }

    ObjectParsingState* clone() {
        ObjectParsingState* newInstance = new ObjectParsingState(schema_object, root);
        newInstance->current_stage = current_stage;
        newInstance->existing_keys = existing_keys;
        newInstance->current_key = current_key;
        newInstance->is_dictionary = is_dictionary;
        return newInstance;
    }

    std::vector<std::string> get_current_possible_keys() const {
        std::vector<std::string> possible_keys;
        if (!is_dictionary) {
            const PropertiesConstraint* propertiesConstraint = findConstraint<PropertiesConstraint>(schema_object);
            for (const auto& key : propertiesConstraint->m_properties) {
                possible_keys.push_back(std::string(key.first));
            }
            for (const auto& key : existing_keys) {
                possible_keys.erase(std::remove(possible_keys.begin(), possible_keys.end(), key), possible_keys.end());
            }
        }
        return possible_keys;
    }

    virtual CharacterLevelParserPtr add_character(char new_character) override { 
        if (new_character == ' ') {
            return shared_from_this();
        }
        
        ObjectParsingState* newState = clone(); // Immutability requirement
        if (current_stage == ObjectParsingStage::START_OBJECT && new_character == '{') {
            newState->current_stage = ObjectParsingStage::PARSING_KEY_OR_END;
        } else if (current_stage == ObjectParsingStage::PARSING_KEY_OR_END) {
            if (new_character == '}') {
                newState->current_stage = ObjectParsingStage::END_OBJECT;
            }
            if (new_character == '"') {
                std::vector<std::string> possible_keys = get_current_possible_keys();
                // We send require_opening_quote=true and then add_character('"') instead of require_opening_quote=false
                // Because there is a difference between "don't need a quote" and "received it before creating the parser"
                CharacterLevelParserPtr key_parser = std::make_shared<StringParsingState>(root, possible_keys, true, true);
                key_parser = key_parser->add_character('"');
                root->context->active_parser->object_stack.push_back(key_parser);
                newState->current_stage = ObjectParsingStage::PARSING_KEY_VALUE_SEPARATOR;
            }
        } else if (current_stage == ObjectParsingStage::PARSING_KEY_VALUE_SEPARATOR) {
            if (new_character == ':') {
                const PropertiesConstraint* propertiesConstraint = findConstraint<PropertiesConstraint>(schema_object);
                newState->current_stage = ObjectParsingStage::PARSING_VALUE;
                newState->current_key = root->context->active_parser->last_parsed_string;
                newState->existing_keys.push_back(newState->current_key);
                std::sort(newState->existing_keys.begin(), newState->existing_keys.end()); // Sorted for std::includes
                if (is_dictionary) {
                    JsonSchemaPtr value_schema;
                    
                    if (propertiesConstraint->m_additionalProperties) {
                        value_schema = propertiesConstraint->m_additionalProperties;
                    } else {
                        value_schema = ANY_JSON_OBJECT_SCHEMA;
                    }
                    CharacterLevelParserPtr current_key_parser = get_parser(root, value_schema);
                    root->context->active_parser->object_stack.push_back(current_key_parser);
                } else {
                    PropertiesConstraint::String key = PropertiesConstraint::String(newState->current_key);
                    JsonSchemaPtr value_schema = propertiesConstraint->m_properties.at(key);
                    CharacterLevelParserPtr current_key_parser = get_parser(root, value_schema);
                    root->context->active_parser->object_stack.push_back(current_key_parser);
                }
            }
        } else if (current_stage == ObjectParsingStage::PARSING_VALUE) {
            // If we receive a character during parsing value, it means that it's the finishing character
            // of the value parser
            if (new_character == '"') {
                newState->current_stage = ObjectParsingStage::PARSING_SEPARATOR_OR_END;
            } else if (new_character == ',') {
                newState->current_stage = ObjectParsingStage::PARSING_KEY_OR_END;
            } else if (new_character == '}') {
                newState->current_stage = ObjectParsingStage::END_OBJECT;
            }
        } else if (current_stage == ObjectParsingStage::PARSING_SEPARATOR_OR_END) {
            if (new_character == ',') {
                newState->current_stage = ObjectParsingStage::PARSING_KEY_OR_END;
            } else if (new_character == '}') {
                newState->current_stage = ObjectParsingStage::END_OBJECT;
            }
        }
        
        return CharacterLevelParserPtr(newState);
    }

    std::string get_allowed_characters() const override { 
        std::vector<char> possible_characters;

        std::vector<std::string> possible_keys = get_current_possible_keys();
        

        const RequiredConstraint* requiredConstraint = findConstraint<RequiredConstraint>(schema_object);
        RequiredConstraint::RequiredProperties required_keys = (requiredConstraint != nullptr) ? requiredConstraint->m_requiredProperties : RequiredConstraint::RequiredProperties();
        std::vector<std::string> required_keys_vector(required_keys.begin(), required_keys.end());
        std::sort(required_keys_vector.begin(), required_keys_vector.end());

        bool can_end = std::includes(existing_keys.begin(), existing_keys.end(), required_keys_vector.begin(), required_keys_vector.end());
        bool can_parse_key = is_dictionary || std::any_of(possible_keys.begin(), possible_keys.end(), [&](const std::string& key) {
            return std::find(existing_keys.begin(), existing_keys.end(), key) == existing_keys.end();
        });

        possible_characters.insert(possible_characters.end(), WHITESPACE_CHARACTERS.begin(), WHITESPACE_CHARACTERS.end());

        if (current_stage == ObjectParsingStage::START_OBJECT) {
            possible_characters.push_back('{');
        }
        else if (current_stage == ObjectParsingStage::PARSING_KEY_OR_END) {
            if (can_end) {
                possible_characters.push_back('}');
            }
            if (can_parse_key) {
                possible_characters.push_back('"');
            }
        }
        else if (current_stage == ObjectParsingStage::PARSING_KEY_VALUE_SEPARATOR) {
            possible_characters.push_back(':');
        }
        else if (current_stage == ObjectParsingStage::PARSING_VALUE) {
            if (can_end) {
                possible_characters.push_back('}');
            }
            if (can_parse_key) {
                possible_characters.push_back(',');
            }
        }
        else if (current_stage == ObjectParsingStage::PARSING_SEPARATOR_OR_END) {
            if (can_end) {
                possible_characters.push_back('}');
            }
            if (can_parse_key) {
                possible_characters.push_back(',');
            }
        }

        return std::string(possible_characters.begin(), possible_characters.end());
    }

    bool can_end() const override { return current_stage == ObjectParsingStage::END_OBJECT; }
};

class ListParsingState : public PrimitiveParsingState {
private:
    JsonSchemaPtr list_member_type;
    bool seen_list_opener;
    bool seen_list_closer;
    size_t num_items_seen;
    size_t min_items;
    size_t max_items;

public:
    ListParsingState(
        JsonSchemaParser* root,
        JsonSchemaPtr list_member_type,
        size_t min_items = -1,
        size_t max_items = -1
    ) : 
        PrimitiveParsingState(root), 
        list_member_type(list_member_type), 
        min_items(min_items), 
        max_items(max_items), 
        num_items_seen(0), 
        seen_list_opener(false), 
        seen_list_closer(false) {
        
    }

    ListParsingState* clone() const {
        ListParsingState* new_state = new ListParsingState(
            this->root,
            this->list_member_type,
            this->min_items,
            this->max_items
        );
        new_state->parsed_string = this->parsed_string;
        new_state->num_items_seen = this->num_items_seen;
        new_state->seen_list_opener = this->seen_list_opener;
        new_state->seen_list_closer = this->seen_list_closer;
        return new_state;
    }

    CharacterLevelParserPtr add_character(char new_character) override {
        CharacterLevelParserPtr base_state = PrimitiveParsingState::add_character(new_character);
        ListParsingState* self = static_cast<ListParsingState*>(base_state.get());
        if (new_character == '[') {
            self->seen_list_opener = true;
            CharacterLevelParserPtr item_parser = get_parser(this->root, this->list_member_type);
            bool requires_items = this->min_items != 0 && this->min_items > 0;
            CharacterLevelParserPtr parser_to_push;
            if (requires_items) {
                parser_to_push = item_parser;
            } else {
                // If we don't require items, we can also end immediately, the Union + ForceStopParser combination achieves this
                std::vector<CharacterLevelParserPtr> parsers;
                parsers.push_back(item_parser);
                parsers.push_back(CharacterLevelParserPtr(new ForceStopParser()));
                parser_to_push = CharacterLevelParserPtr(new UnionParser(parsers));
            }
            this->root->context->active_parser->object_stack.push_back(parser_to_push);
        } else if (new_character == ']') {
            self->seen_list_closer = true;
        } else if (new_character == ',') {
            if (!self->seen_list_closer) {
                self->num_items_seen += 1;
                this->root->context->active_parser->object_stack.push_back(
                    get_parser(
                        this->root,
                        this->list_member_type
                    )
                );
            }
        }
        return base_state;
    }

    std::string get_allowed_characters() const override {
        if (!this->seen_list_opener) {
            return "[" + WHITESPACE_CHARACTERS;
        } else if (!this->seen_list_closer) {
            return this->get_allowed_control_characters() + WHITESPACE_CHARACTERS;
        } else {
            return "";
        }
    }

    bool can_end() const override {
        return this->seen_list_closer;
    }

    std::string get_allowed_control_characters() const {
        int num_items = this->num_items_seen;
        bool is_on_top = this->root->context->active_parser->object_stack.back().get() == this;
        if ((!is_on_top) && this->root->last_non_whitespace_character != "[") {
            // If there is an active parser above us, and the last character is not [, 
            // there is an active item parser on the stack that we did not count yet.
            num_items += 1;
        }
        std::string control_characters = "";
        bool has_enough_items = this->min_items == -1 || num_items >= this->min_items;
        bool can_add_another_item = this->max_items == -1 || num_items < this->max_items;

        if (can_add_another_item) {
            control_characters += ",";
        }
        if (has_enough_items) {
            control_characters += "]";
        }
        return control_characters;
    }
};

CharacterLevelParserPtr get_parser(JsonSchemaParser *parser, const valijson::Subschema *schema)
{
    const AnyOfConstraint* anyOfConstraint = findConstraint<AnyOfConstraint>(schema);
    const TypeConstraint* typeConstraint = findConstraint<TypeConstraint>(schema);
    const EnumConstraint* enumConstraint = findConstraint<EnumConstraint>(schema);
    const PropertiesConstraint* propertiesConstraint = findConstraint<PropertiesConstraint>(schema);
    const PropertyNamesConstraint* propertyNamesConstraint = findConstraint<PropertyNamesConstraint>(schema);
    const RequiredConstraint* requiredConstraint = findConstraint<RequiredConstraint>(schema);

    if (!schema)
    {
        throw std::runtime_error("JsonSchemaParser: Value schema is None");
    }

    if (anyOfConstraint) {
        std::vector<CharacterLevelParserPtr> parsers;
        for (size_t i=0; i<anyOfConstraint->m_subschemas.size(); i++) {
            parsers.push_back(get_parser(parser, anyOfConstraint->m_subschemas.at(i)));
        }
        return CharacterLevelParserPtr(new UnionParser((parsers)));
    }

    if (typeConstraint) {
        size_t num_constraints = typeConstraint->m_namedTypes.size() + typeConstraint->m_schemaTypes.size();
        if (num_constraints != 1) {
            throw std::runtime_error("JsonSchemaParser: TypeConstraint has " + std::to_string(num_constraints) + " constraints, must have only 1");
        }
        if (typeConstraint->m_namedTypes.size() == 1) {
            auto type = *typeConstraint->m_namedTypes.begin();
            switch (type) {
                case TypeConstraint::kString:
                    if (enumConstraint) {
                        std::vector<std::string> enumValues;
                        // TODO: Extract from m_enumValues
                        return CharacterLevelParserPtr(new StringParsingState(parser, enumValues, false, false));
                    } else {
                        return CharacterLevelParserPtr(new StringParsingState(parser, {}, false, false));
                    }
                case TypeConstraint::kInteger:
                    return CharacterLevelParserPtr(new NumberParsingState(parser, false));
                case TypeConstraint::kNumber:
                    return CharacterLevelParserPtr(new NumberParsingState(parser, true));
                case TypeConstraint::kBoolean:
                    return CharacterLevelParserPtr(new StringParsingState(parser, {"true", "false"}, false, false));
                case TypeConstraint::kNull:
                    return CharacterLevelParserPtr(new StringParsingState(parser, {"null"}, false, false));
                case TypeConstraint::kObject:
                    return CharacterLevelParserPtr(new ObjectParsingState(schema, parser));
                case TypeConstraint::kArray:
                {
                    const SingularItemsConstraint* singularItemsConstraint = findConstraint<SingularItemsConstraint>(schema);
                    JsonSchemaPtr list_member_type = (singularItemsConstraint != nullptr) ? singularItemsConstraint->getItemsSubschema() : ANY_JSON_OBJECT_SCHEMA;
                    return CharacterLevelParserPtr(new ListParsingState(parser, list_member_type));
                }
                default:
                    throw std::runtime_error("JsonSchemaParser: Unknown type constraint");
                }
        } else {
            auto schemaType = typeConstraint->m_schemaTypes[0];
        }
    }

    return 0;
}



CharacterLevelParserPtr JsonSchemaParser::add_character(char new_character) {
    int receiving_idx = object_stack.size() - 1;
    std::string last_parsed_string = this->last_parsed_string;
    bool found_receiving_idx = false;
    while (!found_receiving_idx) {
        if (object_stack[receiving_idx]->get_allowed_characters().find(new_character) != std::string::npos) {
            found_receiving_idx = true;
        }
        else {
            CharacterLevelParserPtr finished_receiver = object_stack[receiving_idx];
            if (StringParsingState* string_parser = dynamic_cast<StringParsingState*>(finished_receiver.get())) {
                last_parsed_string = string_parser->parsed_string;
            }
            receiving_idx--;
        }
    }

    std::vector<CharacterLevelParserPtr> updated_stack(object_stack.begin(), object_stack.begin() + receiving_idx + 1);
    JsonSchemaParser* updated_parser = new JsonSchemaParser(context, config, updated_stack, num_consecutive_whitespaces);
    updated_parser->context->active_parser = updated_parser;
    updated_parser->last_parsed_string = last_parsed_string;
    updated_parser->object_stack[receiving_idx] = updated_parser->object_stack[receiving_idx]->add_character(new_character);
    if (std::find(WHITESPACE_CHARACTERS.begin(), WHITESPACE_CHARACTERS.end(), new_character) != WHITESPACE_CHARACTERS.end()) {
        updated_parser->num_consecutive_whitespaces++;
    }
    else {
        updated_parser->num_consecutive_whitespaces = 0;
        updated_parser->last_non_whitespace_character = new_character;
    }
    return CharacterLevelParserPtr(updated_parser);
}

std::string JsonSchemaParser::get_allowed_characters() const {
    std::vector<std::string> allowed_character_strs;
    for (auto it = object_stack.rbegin(); it != object_stack.rend(); ++it) {
        // Similar to SequenceParser, if the top object can end, we need to know to accept the next character of parser below, etc.
        allowed_character_strs.push_back((*it)->get_allowed_characters());
        if (!(*it)->can_end()) {
            break;
        }
    }
    std::string allowed_characters;
    if (allowed_character_strs.size() > 0) {
        allowed_characters = std::accumulate(allowed_character_strs.begin(), allowed_character_strs.end(), std::string());
    }
    else {
        // In certain cases, beam search / sample crashes when there are less legal 
        // continuation tokens than there are beams. Therefore, we allow whitespace 
        // characters when the object stack is empty (= we are done parsing)
        allowed_characters = WHITESPACE_CHARACTERS;
    }

    if (num_consecutive_whitespaces >= MAX_CONSECUTIVE_WHITESPACES) {
        allowed_characters.erase(std::remove_if(allowed_characters.begin(), allowed_characters.end(), [](char c) {
            return std::find(WHITESPACE_CHARACTERS.begin(), WHITESPACE_CHARACTERS.end(), c) != WHITESPACE_CHARACTERS.end();
        }), allowed_characters.end());
    }
    return allowed_characters;
}

bool JsonSchemaParser::can_end() const
{
    for (auto parser : object_stack) {
        if (!parser->can_end()) {
            return false;
        }
    }
    return true;
}