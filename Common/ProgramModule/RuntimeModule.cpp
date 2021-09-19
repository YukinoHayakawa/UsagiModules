#include "RuntimeModule.hpp"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
// must include this header in order to use JIT engine
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>

#include <Usagi/Runtime/ErrorHandling.hpp>

// comments and many code are from https://blog.audio-tk.com/2018/09/18/compiling-c-code-in-memory-with-clang/

namespace usagi
{
void RuntimeModule::create_diagnostics()
{
    auto &compiler_invocation = mCompilerInstance.getInvocation();
    auto &diagnostic_options = compiler_invocation.getDiagnosticOpts();
    auto text_diagnostic_printer =
        std::make_unique<clang::TextDiagnosticPrinter>(
            llvm::errs(),
            &diagnostic_options
        );
    mCompilerInstance.createDiagnostics(
        text_diagnostic_printer.release(),
        true
    );
}

void RuntimeModule::create_invocation()
{
    auto &compiler_invocation = mCompilerInstance.getInvocation();
    auto &diagnostics_engine = mCompilerInstance.getDiagnostics();

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

    if(!clang::CompilerInvocation::CreateFromArgs(
        compiler_invocation,
        item_cstrs,
        diagnostics_engine
    )) USAGI_THROW(std::runtime_error("Failed to create compiler invocation"));
}

void RuntimeModule::setup_input(std::string pch_path, MemoryRegion source)
{
    auto &compiler_invocation = mCompilerInstance.getInvocation();

    // auto *language_options = compiler_invocation.getLangOpts();
    auto &preprocessor_options = compiler_invocation.getPreprocessorOpts();
    auto &target_options = compiler_invocation.getTargetOpts();
    auto &front_end_options = compiler_invocation.getFrontendOpts();

    // bug: this doesn't work with buffers
    // front_end_options.ShowStats = true;
    auto &header_search_options = compiler_invocation.getHeaderSearchOpts();
    // auto &code_gen_options = compiler_invocation.getCodeGenOpts();

    llvm::MemoryBufferRef buffer {
        { (const char*)source.base_address, source.length },
        "<input>"
    };

    // header_search_options.UseStandardSystemIncludes = 0;
    // header_search_options.UseStandardCXXIncludes = 0;
    // header_search_options.AddPath(llvm::StringRef("."), clang::frontend::Quoted, false, true);

    preprocessor_options.ImplicitPCHInclude = pch_path;

    // todo: load from memory - this won't work. PCH include doesn't go this path
    // preprocessor_options.RemappedFileBuffers.emplace("<PCH>", )

    front_end_options.Inputs.push_back(
        clang::FrontendInputFile(buffer, { clang::Language::CXX })
    );

    target_options.Triple = llvm::sys::getDefaultTargetTriple();
}

void RuntimeModule::compile()
{
    llvm::LLVMContext context;

    auto action = std::make_unique<clang::EmitLLVMOnlyAction>(&context);

    if(!mCompilerInstance.ExecuteAction(*action))
        USAGI_THROW(std::runtime_error("Compilation failed."));

    std::unique_ptr<llvm::Module> module = action->takeModule();

    if(!module)
        USAGI_THROW(std::runtime_error("Unable to take IR module."));

    auto &compiler_invocation = mCompilerInstance.getInvocation();
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

    mExecutionEngine.reset(builder.create());
    if(!mExecutionEngine)
        USAGI_THROW(std::runtime_error("Failed to build JIT engine."));
}


std::uint64_t RuntimeModule::get_function_impl(std::string_view name)
{
    const auto addr = mExecutionEngine->getFunctionAddress(std::string(name));
    if(addr == 0)
        USAGI_THROW(std::runtime_error("Function not found."));
    return addr;
}

RuntimeModule::RuntimeModule(std::string pch_path, MemoryRegion source)
{
    create_diagnostics();
    create_invocation();
    setup_input(std::move(pch_path), source);
    compile();
}
}
