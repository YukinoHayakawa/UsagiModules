#include "ClangJIT.hpp"

#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wunused-variable"

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <Usagi/Runtime/WeakSingleton.hpp>

namespace usagi
{
namespace
{
bool gInitCheck = false;
}

void ClangJIT::llvm_init()
{
    assert(gInitCheck == false);

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

    gInitCheck = true;
}

void ClangJIT::llvm_shutdown()
{
    assert(gInitCheck == true);

    llvm::llvm_shutdown();

    gInitCheck = false;
}

ClangJIT::ClangJIT()
    : mLLVM(WeakSingleton<LLVM>::try_lock_construct())
{
}
}
