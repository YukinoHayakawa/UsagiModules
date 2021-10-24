#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Usagi/Runtime/Memory/Region.hpp>

namespace clang
{
class CompilerInstance;
}

namespace llvm::vfs
{
class InMemoryFileSystem;
}

namespace usagi
{
class RuntimeModule;

class CompilerInvocation
{
    // todo output diagnostics to log

    std::unique_ptr<clang::CompilerInstance> mCompilerInstance;
    std::vector<std::string> mStringPool;
    llvm::vfs::InMemoryFileSystem *mFileSystem = nullptr;

    void create_diagnostics();
    void create_vfs();
    void create_invocation();

    // Only allow ClangJIT service to create this class to ensure that LLVM
    // is initialized.
    CompilerInvocation();

    friend class ClangJIT;

public:
    ~CompilerInvocation();

    CompilerInvocation & set_pch(ReadonlyMemoryRegion buffer);
    CompilerInvocation & add_source(
        std::string name,
        ReadonlyMemoryRegion source);
    RuntimeModule compile();
};
}
