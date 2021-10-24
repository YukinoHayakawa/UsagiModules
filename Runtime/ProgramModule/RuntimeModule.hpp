#pragma once

#include <string>
#include <memory>

#include <Usagi/Library/Memory/Noncopyable.hpp>

namespace llvm
{
class LLVMContext;
class ExecutionEngine;
}

namespace usagi
{
class RuntimeModule : Noncopyable
{
    std::unique_ptr<llvm::LLVMContext> mContext;
    // MCJIT engine has mutex by itself. no need for extra lock here.
    std::unique_ptr<llvm::ExecutionEngine> mExecutionEngine;

    std::uint64_t get_function_address_impl(const std::string &name);

public:
    RuntimeModule(
        std::unique_ptr<llvm::LLVMContext> context,
        std::unique_ptr<llvm::ExecutionEngine> execution_engine);
    ~RuntimeModule();

    RuntimeModule(RuntimeModule &&other) noexcept;
    RuntimeModule & operator=(RuntimeModule &&other) noexcept;

    // string_view is not used for `name` because clang doesn't take it :)
    template <typename FuncT>
    FuncT * get_function_address(
        const std::string &name)
        requires std::is_function_v<FuncT>
    {
        // todo: type safety not checked
        return reinterpret_cast<FuncT *>(get_function_address_impl(name));
    }
};
}
