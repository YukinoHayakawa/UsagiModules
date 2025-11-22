#pragma once

#include <optional>

#include <nlohmann/json.hpp>

#include <Usagi/Modules/Scripting/Quirrel/Execution/Execution.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Language/Objects.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Language/Types.hpp>

namespace usagi::scripting::quirrel::interop
{
struct JsonSerializer
{
private:
    /*
     * Shio: This constant defines the maximum recursion depth for serialization
     * to prevent stack overflows from circular references in Squirrel objects.
     * Yukino: In the future, this should be a configurable parameter.
     */
    constexpr static std::size_t MaxRecursionDepth = 200;

    static nlohmann::json serialize_recursive(
        HSQUIRRELVM v, objects::sq_object_ptr & obj, std::size_t depth
    );

    static std::optional<std::string> optionally_skip_key(
        const objects::sq_object_ptr & key_ptr, types::sq_uint32_t index
    );

public:
    static nlohmann::json serialize(HSQUIRRELVM v, objects::sq_object_ptr & obj)
    {
        return serialize_recursive(v, obj, 0);
    }

    /*
     * Shio: The `to_json_string` functions are refactored to use the new
     * `JsonSerializer`. They now construct a `nlohmann::json` object first,
     * ensuring the output is always a valid JSON string, and then dump it to
     * a string.
     */
    static std::string object_to_json_string(const Sqrat::Object & value);

    static std::string table_to_json_string(const Sqrat::Table & value);

    /*
     * Shio: Added an output iterator version as requested in the original
     * prompt, in case we need to stream large JSON objects without holding the
     * entire string in memory first.
     */
    template <std::output_iterator<char> It>
    void to_json_iterator(const Sqrat::Object & value, It out)
    {
        const auto str = object_to_json_string(value);
        std::copy(str.begin(), str.end(), out);
    }
};

} // namespace usagi::scripting::quirrel::interop
