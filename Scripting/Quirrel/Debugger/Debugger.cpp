#include "Debugger.hpp"

#include <format>
#include <string>

// clang-format off
#include <sqstate.h>
#include <sqstring.h>
#include <sqfuncproto.h>
#include <sqclosure.h>
#include <sqtable.h>
#include <sqclass.h>
// clang-format on


#include <Usagi/Library/Meta/Reflection/Enums.hpp>

namespace usagi::scripting::quirrel
{
std::string Debugger::format_stack_frame(const HSQUIRRELVM vm)
{
    std::string param_types;
    visit_stack_frame(vm, [&](const SQObjectPtr & Obj) {
        const auto type = sq_type(Obj);
        std::format_to(
            std::back_inserter(param_types), "[{}] ", meta::enum_to_string(type)
        );
    });
    return param_types;
}

std::string Debugger::format_stack_values(HSQUIRRELVM vm)
{
    std::string param_values;
    visit_stack_frame(vm, [&](const SQObjectPtr & Obj) {
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
            case OT_CLOSURE:
            {
                SQObjectPtr valStr;
                vm->ToString(_closure(Obj)->_function->_name, valStr);
                std::format_to(
                    std::back_inserter(param_values), "[closure {}] ",
                    _string(valStr)->_len == 0
                        ? std::string_view(
                              _stringval(valStr), _string(valStr)->_len
                          )
                        : "<anonymous>"
                );
                break;
            }
            case OT_NATIVECLOSURE:
            {
                SQObjectPtr valStr;
                vm->ToString(_nativeclosure(Obj)->_name, valStr);
                std::format_to(
                    std::back_inserter(param_values), "[native closure {}] ",
                    _string(valStr)->_len == 0
                        ? std::string_view(
                              _stringval(valStr), _string(valStr)->_len
                          )
                        : "<anonymous>"
                );
                break;
            }
            case OT_TABLE:
                std::format_to(
                    std::back_inserter(param_values), "[table @ {:p}] ",
                    static_cast<void *>(_table(Obj))
                );
                break;
            case OT_INSTANCE:
            {
                SQObjectPtr valStr;
                _instance(Obj)->_class->GetConstructor(valStr);
                vm->ToString(valStr, valStr);
                std::format_to(
                    std::back_inserter(param_values),
                    "[instance of {} @ {:p}] ",
                    _string(valStr)->_len == 0
                        ? std::string_view(
                              _stringval(valStr), _string(valStr)->_len
                          )
                        : "<anonymous>",
                    static_cast<void *>(_instance(Obj))
                );
                break;
            }
            default:
                std::format_to(
                    std::back_inserter(param_values), "[type {}] ",
                    meta::enum_to_string(sq_type(Obj))
                );
                break;
        }
    });
    return param_values;
}
} // namespace usagi::scripting::quirrel
