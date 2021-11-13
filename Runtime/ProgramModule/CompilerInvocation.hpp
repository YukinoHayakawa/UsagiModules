#pragma once

#include <memory>
#include <optional>
#include <string>

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
    std::unique_ptr<clang::CompilerInstance> mCompilerInstance;
    llvm::vfs::InMemoryFileSystem *mFileSystem = nullptr;

    std::string mSourceText;

    void create_diagnostics();
    void create_vfs();
    void create_invocation();

    void add_virtual_file(std::string_view name, ReadonlyMemoryRegion bin);

    // Only allow ClangJIT service to create this class to ensure that LLVM
    // is initialized.
    CompilerInvocation();

    friend class ClangJIT;

public:
    ~CompilerInvocation();

    CompilerInvocation & set_pch(
        ReadonlyMemoryRegion source,
        ReadonlyMemoryRegion binary,
        std::optional<std::string> name = { });

    // The name for source is meant for identifying code snippets. They don't
    // necessarily correspond to asset names.
    CompilerInvocation & add_source(
        std::string_view name,
        ReadonlyMemoryRegion source
    );

    RuntimeModule compile();
};
}
