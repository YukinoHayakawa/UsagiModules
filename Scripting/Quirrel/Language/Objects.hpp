#pragma once

union tagSQObjectValue;
struct tagSQObject;
struct SQObjectPtr;

namespace Sqrat
{
class Object;
class Table;
}

namespace usagi::scripting::quirrel::objects
{
using sq_object = tagSQObject;
using sq_object_ptr = SQObjectPtr;
using sq_object_value = tagSQObjectValue;
} // namespace usagi::scripting::quirrel::objects
