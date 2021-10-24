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
#include <llvm/Support/VirtualFileSystem.h>
#include <clang/Basic/FileManager.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Serialization/InMemoryModuleCache.h>

#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Runtime/File/RegularFile.hpp>

#include "RuntimeModule.hpp"

// steps of invocation to clang were learned from https://blog.audio-tk.com/2018/09/18/compiling-c-code-in-memory-with-clang/

namespace usagi
{
void CompilerInvocation::create_diagnostics()
{
    // mCompilerInstance->getModuleCache().addBuiltPCM()
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

void CompilerInvocation::create_vfs()
{
    // use a vfs to handle the reading of PCH since it only reads from the
    // file manager.
    mCompilerInstance->setFileManager(
        new clang::FileManager(
            { },
            mFileSystem = new llvm::vfs::InMemoryFileSystem
        )
    );
}

void CompilerInvocation::create_invocation()
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &diagnostics_engine = mCompilerInstance->getDiagnostics();
    auto &target_options = compiler_invocation.getTargetOpts();
    target_options.Triple = llvm::sys::getDefaultTargetTriple();

    if(!clang::CompilerInvocation::CreateFromArgs(
        compiler_invocation,
        { nullptr, nullptr },
        diagnostics_engine
    )) USAGI_THROW(std::runtime_error("Failed to create compiler invocation"));
}

CompilerInvocation::CompilerInvocation()
    : mCompilerInstance { std::make_unique<clang::CompilerInstance>() }
{
    create_diagnostics();
    create_vfs();
    create_invocation();
}

CompilerInvocation::~CompilerInvocation()
{
}

CompilerInvocation & CompilerInvocation::set_pch(ReadonlyMemoryRegion buffer)
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &preprocessor_options = compiler_invocation.getPreprocessorOpts();

    preprocessor_options.ImplicitPCHInclude = "<pch>";
    // prevent clang from checking the pch with original header source (we
    // don't have it in the vfs and don't want to deploy it with assets)
    preprocessor_options.DisablePCHOrModuleValidation =
        clang::DisableValidationForModuleKind::PCH;

    mFileSystem->addFile(
        "<pch>",
        0,
        llvm::MemoryBuffer::getMemBuffer(
            llvm::StringRef
            { (const char *)buffer.base_address, buffer.length },
            "",
            false
        )
    );

    return *this;
}

CompilerInvocation & CompilerInvocation::add_source(
    std::string name,
    ReadonlyMemoryRegion source)
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &front_end_options = compiler_invocation.getFrontendOpts();

    mStringPool.push_back(std::move(name));

    llvm::MemoryBufferRef buffer {
        llvm::StringRef { (const char*)source.base_address, source.length },
        mStringPool.back()
    };

    // header_search_options.UseStandardSystemIncludes = 0;
    // header_search_options.UseStandardCXXIncludes = 0;
    // header_search_options.AddPath(llvm::StringRef("."), clang::frontend::Quoted, false, true);

    // remove reading from stdin
    front_end_options.Inputs.clear();
    front_end_options.Inputs.push_back(
        clang::FrontendInputFile(buffer, { clang::Language::CXX })
    );

    return *this;
}

RuntimeModule CompilerInvocation::compile()
{
    assert(mCompilerInstance);

    auto context = std::make_unique<llvm::LLVMContext>();
    auto action = std::make_unique<clang::EmitLLVMOnlyAction>(context.get());

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

    return {
        std::move(context),
        std::move(engine)
    };
}
}
