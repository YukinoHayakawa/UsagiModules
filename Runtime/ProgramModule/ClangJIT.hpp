﻿#pragma once

#include <Usagi/Library/Memory/RawResource.hpp>

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

public:
    ClangJIT();

    // To make sure that LLVM is instantiated, this should be not declared
    // as static.
    auto create_compiler()
    {
        return CompilerInvocation();
    }
};
}
