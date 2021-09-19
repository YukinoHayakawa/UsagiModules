#pragma once

#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wunused-variable"

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <string_view>
#include <type_traits>

#include <llvm/IR/LLVMContext.h>
#include <clang/Frontend/CompilerInstance.h>

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Runtime/Memory/Region.hpp>

namespace llvm
{
class ExecutionEngine;
}

namespace usagi
{
class RuntimeModule : Noncopyable
{
    clang::CompilerInstance mCompilerInstance;
    std::unique_ptr<llvm::ExecutionEngine> mExecutionEngine;

    void create_diagnostics();
    void create_invocation();
    void setup_input(std::string pch_path, MemoryRegion source);
    void compile();

    std::uint64_t get_function_impl(std::string_view name);

public:
    RuntimeModule(std::string pch_path, MemoryRegion source);

    template <typename FuncT>
    FuncT get_function(std::string_view name)
        requires std::is_function_v<FuncT>
    {
        // warning: type safety not checked
        return reinterpret_cast<FuncT>(get_function_impl(name));
    }
};
}
