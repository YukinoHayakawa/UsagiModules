#pragma once

#include <string>
#include <memory>
#include <optional>

#include <Usagi/Library/Memory/Noncopyable.hpp>

namespace llvm
{
class Module;
class LLVMContext;
class ExecutionEngine;
}

namespace usagi
{
// todo: memory mgmt? are objects properly destructed when the script it unloaded?
class RuntimeModule : Noncopyable
{
    std::unique_ptr<llvm::LLVMContext> mContext;
    // MCJIT engine has mutex by itself. no need for extra lock here.
    std::unique_ptr<llvm::ExecutionEngine> mExecutionEngine;
    llvm::Module *mModule = nullptr;

    std::uint64_t get_function_address_impl(const std::string &name);

public:
    RuntimeModule(
        std::unique_ptr<llvm::LLVMContext> context,
        std::unique_ptr<llvm::ExecutionEngine> execution_engine,
        llvm::Module *module);
    ~RuntimeModule();

    RuntimeModule(RuntimeModule &&other) noexcept;
    RuntimeModule & operator=(RuntimeModule &&other) noexcept;

    [[nodiscard]]
    std::optional<std::string> search_function(std::string_view name);

    // string_view is not used for `name` due to the function signature found
    // in original clang source only accepts a const string &.
    template <typename FuncPtrT>
    FuncPtrT get_function_address(
        const std::string &name)
        requires std::is_function_v<std::remove_pointer_t<FuncPtrT>>
    {
        // todo: type safety not checked
        return reinterpret_cast<FuncPtrT>(get_function_address_impl(name));
    }
};
}
