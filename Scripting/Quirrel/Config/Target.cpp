#include <cstdint>

#include <sqconfig.h>

namespace usagi::scripting::quirrel
{
// Traits

static_assert(SQ_ALIGNMENT == sizeof(std::uint64_t));
} // namespace usagi::scripting::quirrel
