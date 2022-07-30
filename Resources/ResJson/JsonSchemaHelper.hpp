#pragma once

// Macros for translating the enums

// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
#define COMMON_HEADER(_key, _dst) \
    do { \
        const char *key = _key; \
        auto &node = config[nlohmann::json::json_pointer(key)]; \
        [[maybe_unused]] \
        auto &dst = _dst; \
/**/

#define FIELD(_key, _dst, _enum_type) \
    COMMON_HEADER(_key, _dst) \
        using EnumT = _enum_type;
#define CHECK_TYPE(_node, _key, _type) \
    if(!(_node).is_##_type()) \
        USAGI_THROW(std::runtime_error( \
            std::format("Unexpected type for key {}: " \
            "should be a {}, but received a {} instead.", \
            _key, #_type, (_node).type_name()) \
        ));
// Begin with either MATCH_NULL or MATCH, so `value` is defined.
#define MATCH_NULL(_enum_val) \
    if(node.is_null()) dst = EnumT::_enum_val; else
#define MATCH(_str, _enum_val) \
    if(std::string value = node; value == (_str)) dst = EnumT::_enum_val;
#define ELSE_(_str, _enum_val) \
    else if(value == (_str)) dst = EnumT::_enum_val;
#define END_FIELD() \
    else \
        USAGI_THROW(std::runtime_error( \
            std::format("Invalid value for key {}: {}", key, value) \
        )); \
    } while(false); \
/**/

#define MATCH_BOOL(_key, _dst) \
    COMMON_HEADER(_key, _dst); \
        CHECK_TYPE(node, key, boolean) \
        (_dst) = node.get<bool>(); \
    } while(false); \
/**/
// ReSharper enable CppClangTidyCppcoreguidelinesMacroUsage
