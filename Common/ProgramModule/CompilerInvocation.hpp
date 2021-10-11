#pragma once

#include <memory>
#include <string>

#include <Usagi/Runtime/Memory/Region.hpp>

namespace clang
{
class CompilerInstance;
}

namespace usagi
{
class RuntimeModule;

class CompilerInvocation
{
    // todo output diagnostics to log

    std::unique_ptr<clang::CompilerInstance> mCompilerInstance;

    void create_diagnostics();
    void create_invocation();

    // Only allow ClangJIT service to create this class to ensure that LLVM
    // is initialized.
    CompilerInvocation();
    ~CompilerInvocation();

    friend class ClangJIT;

public:
    CompilerInvocation & set_pch(std::string path);
    CompilerInvocation & add_source(std::string name, MemoryRegion source);
    std::unique_ptr<RuntimeModule> compile();
};
}
