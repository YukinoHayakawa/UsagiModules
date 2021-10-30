#pragma once

#include <Usagi/Runtime/Service/SimpleService.hpp>

#include "ClangJIT.hpp"

namespace usagi
{
using ServiceJitCompilation = SimpleService<ClangJIT>;
}
