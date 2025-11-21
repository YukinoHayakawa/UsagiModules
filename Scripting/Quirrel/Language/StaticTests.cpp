#include <type_traits>

#include <squirrel.h>

#include "Types.hpp"

// This static test file is not in the header because we don't want to expose
// Querrel headers to the end users.
namespace usagi::scripting::quirrel::static_tests
{
static_assert(_SQ64, "Why on earth would you compile a 32-bit program?");
static_assert(SQ_ALIGNMENT == 8);

// The following tests are in the same order as the types are defined in
// `<sqconfig.h>`. Namespace `types` is intentionally used here to avoid any
// potential ambiguity.

static_assert(std::is_same_v<types::sq_int_t, SQInteger>);
static_assert(std::is_same_v<types::sq_uint_t, SQUnsignedInteger>);
static_assert(std::is_same_v<types::hash_t, SQHash>);

static_assert(std::is_same_v<types::sq_int32_t, SQInt32>);
static_assert(std::is_same_v<types::sq_uint32_t, SQUnsignedInteger32>);
static_assert(std::is_same_v<types::sq_float_t, SQFloat>);
static_assert(std::is_same_v<types::sq_bool_t, SQBool>);
static_assert(std::is_same_v<types::sq_char_t, SQChar>);

static_assert(std::is_same_v<types::raw_object_val_t, SQRawObjectVal>);
static_assert(std::is_same_v<types::user_pointer_t, SQUserPointer>);
static_assert(std::is_same_v<types::result_t, SQRESULT>);
} // namespace usagi::scripting::quirrel::static_tests
