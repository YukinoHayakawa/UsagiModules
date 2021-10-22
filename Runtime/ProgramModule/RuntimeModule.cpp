// suppress llvm-related warnings
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wunused-variable"

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "RuntimeModule.hpp"

// must include this header in order to use JIT engine
#include <llvm/ExecutionEngine/MCJIT.h>

#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
std::uint64_t RuntimeModule::get_function_address_impl(const std::string &name)
{
    const auto addr = mExecutionEngine->getFunctionAddress(name);
    if(addr == 0)
        USAGI_THROW(std::runtime_error("Function not found."));
    return addr;
}

RuntimeModule::RuntimeModule(
    std::unique_ptr<llvm::LLVMContext> context,
    std::unique_ptr<llvm::ExecutionEngine> execution_engine)
    : mContext(std::move(context))
    , mExecutionEngine(std::move(execution_engine))
{
}

RuntimeModule::~RuntimeModule()
{
}
}
