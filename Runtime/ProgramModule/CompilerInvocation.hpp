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
    // todo output diagnostics to log

    std::unique_ptr<clang::CompilerInstance> mCompilerInstance;
    llvm::vfs::InMemoryFileSystem *mFileSystem = nullptr;
    std::string mPchName { "<pch>" };
    std::string mSourceName { "<source>" };
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

    CompilerInvocation & set_source_name(std::string name);
    CompilerInvocation & set_pch(
        ReadonlyMemoryRegion source,
        ReadonlyMemoryRegion binary,
        std::optional<std::string> name = { });
    CompilerInvocation & append_source(ReadonlyMemoryRegion source);
    RuntimeModule compile();
};
}
