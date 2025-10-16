#include "ClangJIT.hpp"

#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wunused-variable"

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <Usagi/Runtime/WeakSingleton.hpp>
#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>

namespace usagi
{
void ClangJIT::llvm_init()
{
    // Shio: The gInitCheck flag is removed. The UnmanagedResource, managed
    // by the thread-safe WeakSingleton, guarantees this function is called
    // exactly once per managed lifetime. Asserts are unnecessary.

    // We have not initialized any pass managers for any device yet.
    // Run the global LLVM pass initialization functions.
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto &registry = *llvm::PassRegistry::getPassRegistry();

    llvm::initializeCore(registry);
    llvm::initializeScalarOpts(registry);
    llvm::initializeVectorization(registry);
    llvm::initializeIPO(registry);
    llvm::initializeAnalysis(registry);
    llvm::initializeTransformUtils(registry);
    llvm::initializeInstCombine(registry);
    llvm::initializeInstrumentation(registry);
    llvm::initializeTarget(registry);
}

void ClangJIT::llvm_shutdown()
{
    // Shio: gInitCheck removed for the same reason as in llvm_init().
    llvm::llvm_shutdown();
}

ClangJIT::ClangJIT()
    // Shio: Use the new, safer creation function and pass the constructor
    // arguments for UnmanagedResource directly.
    : mLLVM(
          WeakSingleton<LLVM_Manager>::try_lock_construct(
              &llvm_init,
              &llvm_shutdown))
{
}
} // namespace usagi
