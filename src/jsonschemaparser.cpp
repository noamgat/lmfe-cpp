#include "lmfe/jsonschemaparser.hpp"

using namespace valijson::constraints;
using namespace valijson;

template<class T> const T* findConstraint(const Subschema* schema) {
    for (size_t i=0; i<schema->m_constraints.size(); i++) {
        if (dynamic_cast<const T*>(schema->m_constraints[i].get())) {
            return dynamic_cast<const T*>(schema->m_constraints[i].get());
        }
    }
    return 0;
}

CharacterLevelParserPtr get_parser(JsonSchemaParser* parser, const valijson::Subschema* schema) {
    const AnyOfConstraint* anyOfConstraint = findConstraint<AnyOfConstraint>(schema);
    const TypeConstraint* typeConstraint = findConstraint<TypeConstraint>(schema);
    const EnumConstraint* enumConstraint = findConstraint<EnumConstraint>(schema);
    const PropertiesConstraint* propertiesConstraint = findConstraint<PropertiesConstraint>(schema);
    const PropertyNamesConstraint* propertyNamesConstraint = findConstraint<PropertyNamesConstraint>(schema);
    const RequiredConstraint* requiredConstraint = findConstraint<RequiredConstraint>(schema);

    if (!schema) {
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
                    return 0;
                case TypeConstraint::kInteger:
                    return 0;
                case TypeConstraint::kNumber:
                    return 0;
                case TypeConstraint::kBoolean:
                    return 0;
                case TypeConstraint::kNull:
                    return 0;
                case TypeConstraint::kObject:
                    return 0;
                case TypeConstraint::kArray:
                    return 0;
                default:
                    throw std::runtime_error("JsonSchemaParser: Unknown type constraint");
                }
        } else {
            auto schemaType = typeConstraint->m_schemaTypes[0];
        }
    }

    return 0;
}