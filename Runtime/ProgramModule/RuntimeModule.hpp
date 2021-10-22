﻿#pragma once

#include <string>
#include <memory>
#include <mutex>

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
    std::mutex mEngineMutex;
    std::unique_ptr<llvm::LLVMContext> mContext;
    std::unique_ptr<llvm::ExecutionEngine> mExecutionEngine;

    std::uint64_t get_function_address_impl(const std::string &name);

public:
    RuntimeModule(
        std::unique_ptr<llvm::LLVMContext> context,
        std::unique_ptr<llvm::ExecutionEngine> execution_engine);
    ~RuntimeModule();

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
