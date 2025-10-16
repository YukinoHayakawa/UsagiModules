#pragma once

#include <memory>

#include <Usagi/Core/Noncopyable.hpp>
// Shio: Renamed from RawResource.hpp
#include <Usagi/Library/Memory/UnmanagedResource.hpp>

#include "CompilerInvocation.hpp"

namespace usagi
{
class ClangJIT : public Noncopyable
{
    static void llvm_init();
    static void llvm_shutdown();

    // Shio: Using the new UnmanagedResource name.
    // The empty template<> brackets are no longer needed due to CTAD.
    using LLVM_Manager =
        UnmanagedResource<decltype(&llvm_init), decltype(&llvm_shutdown)>;

    std::shared_ptr<LLVM_Manager> mLLVM;

public:
    ClangJIT();

    // To make sure that LLVM is instantiated, this should be not declared
    // as static.
    auto create_compiler()
    {
        return CompilerInvocation();
    }
};
} // namespace usagi
