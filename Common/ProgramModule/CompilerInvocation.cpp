// suppress llvm-related warnings
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wunused-variable"

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "CompilerInvocation.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/Host.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>

#include <Usagi/Runtime/ErrorHandling.hpp>

#include "RuntimeModule.hpp"

// steps of invocation to clang were learned from https://blog.audio-tk.com/2018/09/18/compiling-c-code-in-memory-with-clang/

namespace usagi
{
void CompilerInvocation::create_diagnostics()
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &diagnostic_options = compiler_invocation.getDiagnosticOpts();
    auto text_diagnostic_printer =
        std::make_unique<clang::TextDiagnosticPrinter>(
            llvm::errs(),
            &diagnostic_options
            );
    mCompilerInstance->createDiagnostics(
        text_diagnostic_printer.release(),
        true
    );
}

void CompilerInvocation::create_invocation()
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &diagnostics_engine = mCompilerInstance->getDiagnostics();
    auto &target_options = compiler_invocation.getTargetOpts();

    /* todo seems no need to specify target this way since target option can be directly assigned
    std::stringstream ss;
    ss << "-triple=" << llvm::sys::getDefaultTargetTriple();

    llvm::SmallVector<std::string, 128> item_strs;
    llvm::SmallVector<const char *, 128> item_cstrs;

    for(std::istream_iterator<std::string> i(ss), end; i != end; ++i)
    {
        item_strs.push_back(*i);
    }

    item_cstrs.reserve(item_strs.size());
    std::ranges::transform(
        item_strs,
        std::back_inserter(item_cstrs),
        [](const std::string &str) { return str.c_str(); }
    );
    */

    if(!clang::CompilerInvocation::CreateFromArgs(
        compiler_invocation,
        { nullptr, nullptr },
        diagnostics_engine
    )) USAGI_THROW(std::runtime_error("Failed to create compiler invocation"));

    target_options.Triple = llvm::sys::getDefaultTargetTriple();
}

CompilerInvocation::CompilerInvocation()
{
    create_diagnostics();
    create_invocation();
}

CompilerInvocation::~CompilerInvocation()
{
}

CompilerInvocation & CompilerInvocation::set_pch(std::string path)
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &preprocessor_options = compiler_invocation.getPreprocessorOpts();

    preprocessor_options.ImplicitPCHInclude = path;

    return *this;
}

CompilerInvocation & CompilerInvocation::add_source(
    std::string name,
    MemoryRegion source)
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &front_end_options = compiler_invocation.getFrontendOpts();

    llvm::MemoryBufferRef buffer {
        { (const char*)source.base_address, source.length },
        name
    };

    // header_search_options.UseStandardSystemIncludes = 0;
    // header_search_options.UseStandardCXXIncludes = 0;
    // header_search_options.AddPath(llvm::StringRef("."), clang::frontend::Quoted, false, true);

    // todo: load from memory - this won't work. PCH include doesn't go this path
    // preprocessor_options.RemappedFileBuffers.emplace("<PCH>", )

    front_end_options.Inputs.push_back(
        clang::FrontendInputFile(buffer, { clang::Language::CXX })
    );

    return *this;
}

std::unique_ptr<RuntimeModule> CompilerInvocation::compile()
{
    assert(mCompilerInstance);

    llvm::LLVMContext context;

    auto action = std::make_unique<clang::EmitLLVMOnlyAction>(&context);

    if(!mCompilerInstance->ExecuteAction(*action))
        USAGI_THROW(std::runtime_error("Compilation failed."));

    std::unique_ptr<llvm::Module> module = action->takeModule();

    if(!module)
        USAGI_THROW(std::runtime_error("Unable to take IR module."));

    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &code_gen_options = compiler_invocation.getCodeGenOpts();

    llvm::PassBuilder pass_builder;
    llvm::LoopAnalysisManager loop_analysis_manager(
        code_gen_options.DebugPassManager
    );
    llvm::FunctionAnalysisManager function_analysis_manager(
        code_gen_options.DebugPassManager
    );
    llvm::CGSCCAnalysisManager cgscc_analysis_manager(
        code_gen_options.DebugPassManager
    );
    llvm::ModuleAnalysisManager module_analysis_manager(
        code_gen_options.DebugPassManager
    );

    pass_builder.registerModuleAnalyses(module_analysis_manager);
    pass_builder.registerCGSCCAnalyses(cgscc_analysis_manager);
    pass_builder.registerFunctionAnalyses(function_analysis_manager);
    pass_builder.registerLoopAnalyses(loop_analysis_manager);
    pass_builder.crossRegisterProxies(
        loop_analysis_manager,
        function_analysis_manager,
        cgscc_analysis_manager,
        module_analysis_manager
    );

    // do the optimization
    llvm::ModulePassManager module_pass_manager =
        pass_builder.buildPerModuleDefaultPipeline(
            llvm::PassBuilder::OptimizationLevel::O3
        );
    module_pass_manager.run(*module, module_analysis_manager);

    llvm::EngineBuilder builder(std::move(module));
    builder.setMCJITMemoryManager(
        std::make_unique<llvm::SectionMemoryManager>()
    );
    builder.setOptLevel(llvm::CodeGenOpt::Level::Aggressive);
    builder.setEngineKind(llvm::EngineKind::JIT);

    std::unique_ptr<llvm::ExecutionEngine> engine(builder.create());
    if(!engine)
        USAGI_THROW(std::runtime_error("Failed to build JIT engine."));

    mCompilerInstance.reset();

    return std::make_unique<RuntimeModule>(std::move(engine));
}
}
