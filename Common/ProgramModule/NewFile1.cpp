#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wunused-variable"


#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <ranges>

#include <llvm/InitializePasses.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
// must include this header in order to use JIT engine
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>

#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>

bool LLVMinit = false;

// code based on https://blog.audio-tk.com/2018/09/18/compiling-c-code-in-memory-with-clang/
void InitializeLLVM()
{
    if(LLVMinit) { return; }

    // We have not initialized any pass managers for any device yet.
    // Run the global LLVM pass initialization functions.
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto &Registry = *llvm::PassRegistry::getPassRegistry();

    llvm::initializeCore(Registry);
    llvm::initializeScalarOpts(Registry);
    llvm::initializeVectorization(Registry);
    llvm::initializeIPO(Registry);
    llvm::initializeAnalysis(Registry);
    llvm::initializeTransformUtils(Registry);
    llvm::initializeInstCombine(Registry);
    llvm::initializeInstrumentation(Registry);
    llvm::initializeTarget(Registry);

    LLVMinit = true;
}

//void (*compile_and_run(std::string_view source, const std::string &function_name))()
int compile_and_run(std::string_view source, const std::string &function_name)
{
    InitializeLLVM();


    clang::CompilerInstance compiler_instance;


    // Let’s start by setting up the compiler diagnostics (where every warning and error will be written) and then the compiler instance and its invocation. The invocation is what we will actually use to compile our code.

    auto &compiler_invocation = compiler_instance.getInvocation();
    auto &diagnostic_options = compiler_invocation.getDiagnosticOpts();

    auto text_diagnostic_printer =
        std::make_unique<clang::TextDiagnosticPrinter>(
            llvm::errs(),
            &diagnostic_options
        );
    compiler_instance.getDiagnosticOpts();
    compiler_instance.createDiagnostics(
        text_diagnostic_printer.release(),
        true
    );
    auto &diagnostics_engine = compiler_instance.getDiagnostics();

    // Let’s now create our arguments. Here, I’m only setting up the “triple” which is the target platform. But this is where I will add all the include paths later



    // todo insert pch/headers etc
    std::stringstream ss;
    ss << "-triple=" << llvm::sys::getDefaultTargetTriple();

    std::vector<std::string> itemstrs;
    std::vector<const char *> itemcstrs;

    for(std::istream_iterator<std::string> i(ss), end; i != end; ++i)
    {
        itemstrs.push_back(*i);
    }

    itemcstrs.reserve(itemstrs.size());
    std::ranges::transform(
        itemstrs,
        std::back_inserter(itemcstrs),
        [](const std::string &str) { return str.c_str(); }
    );

    if(!clang::CompilerInvocation::CreateFromArgs(
        compiler_invocation,
        itemcstrs,
        diagnostics_engine
    )) throw;

//    Now that the compiler invocation is done, we can tweak the options.

    auto *language_options = compiler_invocation.getLangOpts();
    auto &preprocessor_options = compiler_invocation.getPreprocessorOpts();
    auto &target_options = compiler_invocation.getTargetOpts();
    auto &front_end_options = compiler_invocation.getFrontendOpts();
#ifndef _NDEBUG
    // this doesn't work with buffers
//    front_end_options.ShowStats = true;
#endif
    auto &header_search_options = compiler_invocation.getHeaderSearchOpts();
#ifndef _NDEBUG
    header_search_options.Verbose = true;
#endif
    auto &code_gen_options = compiler_invocation.getCodeGenOpts();

//    And now we are set up to compile our file :

    llvm::MemoryBufferRef buffer {
        { source.data(), source.size() },
        "<input>"
    };

//    header_search_options.UseStandardSystemIncludes = 0;
//    header_search_options.UseStandardCXXIncludes = 0;
//    header_search_options.AddPath(llvm::StringRef("D:/Private/Dev/Projects/UsagiBuild/x64/Debug"), clang::frontend::Quoted, false, true);
    preprocessor_options.ImplicitPCHInclude = "foo.pch";
    // todo: load from memorys
    //    preprocessor_options.RemappedFileBuffers.emplace("<PCH>", )

    front_end_options.Inputs.clear();
    front_end_options.Inputs.push_back(
        clang::FrontendInputFile(buffer, { clang::Language::CXX })
    );

    target_options.Triple = llvm::sys::getDefaultTargetTriple();

    llvm::LLVMContext context;
    auto action = std::make_unique<clang::EmitLLVMOnlyAction>(&context);

    if(!compiler_instance.ExecuteAction(*action))
    {
        // error happened
//        diagnosticsEngine->dump();
        throw;
    }

//    The first step is to get the IR module from the previous action.

    std::unique_ptr<llvm::Module> module = action->takeModule();

    if(!module)
    {
        throw;
    }


//    We will now make the different optimization passes.The code to create the passes is rather complicated andcreates more stuff that s required, but this is LLVM… andone of the reasons the API keeps on mutating.

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

//    We can now use the JIT compiler andextract the function we need.Be aware that the engine needs to stay alive as long as you use the function :

    llvm::EngineBuilder builder(std::move(module));
    builder.setMCJITMemoryManager(
        std::make_unique<llvm::SectionMemoryManager>()
    );
    builder.setOptLevel(llvm::CodeGenOpt::Level::Aggressive);
    builder.setEngineKind(llvm::EngineKind::JIT);
    auto execution_engine = builder.create();

    if(!execution_engine)
    {
        throw;
    }

    auto func = reinterpret_cast<int(*)()>(
        execution_engine->getFunctionAddress(function_name)
    );
    return func();
}
