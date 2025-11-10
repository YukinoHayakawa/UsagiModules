#include "Debugger.hpp"

#include <format>
#include <string>

#include <sqstring.h>

#include <Usagi/Library/Meta/Reflection/Enums.hpp>

namespace usagi::scripting::quirrel
{
std::string Debugger::format_stack_frame(const HSQUIRRELVM vm)
{
    std::string param_types;
    visit_stack_frame(
        vm,
        [&](const SQObjectPtr & Obj) {
            const auto type = sq_type(Obj);
            std::format_to(
                std::back_inserter(param_types), "[{}] ",
                meta::enum_to_string(type)
            );
        }
    );
    return param_types;
}

std::string Debugger::format_stack_values(HSQUIRRELVM vm)
{
    std::string param_values;
    visit_stack_frame(
        vm,
        [&](const SQObjectPtr & Obj) {
            switch(sq_type(Obj))
            {
                case OT_NULL:
                    std::format_to(std::back_inserter(param_values), "[null] ");
                    break;
                case OT_INTEGER:
                    std::format_to(
                        std::back_inserter(param_values), "[{}] ", _integer(Obj)
                    );
                    break;
                case OT_FLOAT:
                    std::format_to(
                        std::back_inserter(param_values), "[{}] ", _float(Obj)
                    );
                    break;
                case OT_STRING:
                    std::format_to(
                        std::back_inserter(param_values), "[\"{}\"] ",
                        std::string_view(_stringval(Obj), _string(Obj)->_len)
                    );
                    break;
                case OT_BOOL:
                    std::format_to(
                        std::back_inserter(param_values), "[{}] ",
                        _integer(Obj) ? "true" : "false"
                    );
                    break;
                default:
                    std::format_to(
                        std::back_inserter(param_values), "[type {}] ",
                        meta::enum_to_string(sq_type(Obj))
                    );
                    break;
            }
        }
    );
    return param_values;
}
} // namespace usagi::scripting::quirrel
