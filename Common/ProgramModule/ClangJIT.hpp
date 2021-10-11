#pragma once

#include <Usagi/Library/Memory/RawResource.hpp>
#include <Usagi/Runtime/Memory/Region.hpp>

#include "CompilerInvocation.hpp"

namespace usagi
{
class RuntimeModule;

class ClangJIT : Noncopyable
{
    static void llvm_init();
    static void llvm_shutdown();

    struct LLVM : RawResource<>
    {
        LLVM() : RawResource(&llvm_init, &llvm_shutdown)
        {
        }
    };

    std::shared_ptr<LLVM> mLLVM;

    void find_module_or_compile(
        std::string_view module_name,
        std::string_view pch_path,
        MemoryRegion source);

public:
    ClangJIT();

    static CompilerInvocation create_compiler()
    {
        return CompilerInvocation();
    }
};
}
