// suppress llvm-related warnings
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wunused-variable"

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "CompilerInvocation.hpp"

#include <iostream>
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

    std::stringstream ss;
    // ss << "-triple=" << llvm::sys::getDefaultTargetTriple();
    ss << "-cc1 -triple x86_64-pc-windows-msvc19.29.30136 -mrelax-all -mincremental-linker-compatible --mrelax-relocations -disable-free -disable-llvm-verifier -discard-value-names -mrelocation-model pic -pic-level 2 -mframe-pointer=none -relaxed-aliasing -fmath-errno -fno-rounding-math -mconstructor-aliases -munwind-tables -target-cpu haswell -mllvm -x86-asm-syntax=intel -D_DEBUG -D_MT -flto-visibility-public-std --dependent-lib=libcmtd --dependent-lib=oldnames -stack-protector 2 -fcxx-exceptions -fexceptions -fexternc-nounwind -fms-volatile -fdefault-calling-conv=cdecl -fdiagnostics-format msvc -gno-column-info -gcodeview -debug-info-kind=limited -D _UNICODE -D UNICODE -D _HAS_CXX17 -D _HAS_CXX20  -O0 -Wall -Werror -Wno-pragma-pack -fdeprecated-macro -ferror-limit 19 -fno-use-cxa-atexit -fms-extensions -fms-compatibility -fms-compatibility-version=19.29.30136 -std=c++20 -fdelayed-template-parsing -fno-implicit-modules -fno-caret-diagnostics -std=c++20 -faddrsig -x c++";

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
        // { nullptr, nullptr },
        item_cstrs,
        diagnostics_engine
    )) USAGI_THROW(std::runtime_error("Failed to create compiler invocation"));

    // remove reading from stdin
    auto &front_end_options = compiler_invocation.getFrontendOpts();
    front_end_options.Inputs.clear();
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
    // // prevent clang from checking the pch with original header source (we
    // // don't have it in the vfs and don't want to deploy it with assets)
    // preprocessor_options.DisablePCHOrModuleValidation =
    // clang::DisableValidationForModuleKind::PCH;
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


    const auto path = "D:\\Private\\Dev\\Projects\\UsagiBuild\\Demos\\DemoScripting\\Database.i";
    auto file = new RegularFile(path);
    auto view = new MappedFileView(file->create_view());
    auto region = view->memory_region();
    // auto buf = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef { (const char *)region.base_address, region.length }).release();
    auto file_time = std::filesystem::last_write_time(path);
    const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
    const auto time = std::chrono::system_clock::to_time_t(systemTime);

    // preprocessor_options.addRemappedFile(path, buf);
    mFileSystem->addFile(
        path,
        // 0,
        time,
        llvm::MemoryBuffer::getMemBuffer(
            llvm::StringRef
            { (const char *)region.base_address, region.length },
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
    src += std::string_view((const char *)source.base_address, source.length);
    //
    // auto &compiler_invocation = mCompilerInstance->getInvocation();
    // auto &front_end_options = compiler_invocation.getFrontendOpts();
    //
    // mStringPool.push_back(std::move(name));
    //
    // llvm::MemoryBufferRef buffer {
    //     llvm::StringRef { (const char*)source.base_address, source.length },
    //     mStringPool.back()
    // };
    //
    // // header_search_options.UseStandardSystemIncludes = 0;
    // // header_search_options.UseStandardCXXIncludes = 0;
    // // header_search_options.AddPath(llvm::StringRef("."), clang::frontend::Quoted, false, true);
    //
    // front_end_options.Inputs.push_back(
    //     clang::FrontendInputFile(buffer, { clang::Language::CXX })
    // );

    return *this;
}

RuntimeModule CompilerInvocation::compile()
{
    assert(mCompilerInstance);


    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &front_end_options = compiler_invocation.getFrontendOpts();

    mStringPool.push_back("source");

    llvm::MemoryBufferRef buffer {
        src,
        mStringPool.back()
    };

    // header_search_options.UseStandardSystemIncludes = 0;
    // header_search_options.UseStandardCXXIncludes = 0;
    // header_search_options.AddPath(llvm::StringRef("."), clang::frontend::Quoted, false, true);

    front_end_options.Inputs.push_back(
        clang::FrontendInputFile(buffer, { clang::Language::CXX })
    );

    auto context = std::make_unique<llvm::LLVMContext>();
    auto action = std::make_unique<clang::EmitLLVMOnlyAction>(context.get());

    if(!mCompilerInstance->ExecuteAction(*action))
        USAGI_THROW(std::runtime_error("Compilation failed."));

    std::unique_ptr<llvm::Module> module = action->takeModule();

    if(!module)
        USAGI_THROW(std::runtime_error("Unable to take IR module."));

    for(auto &&func : module->getFunctionList())
    {
        std::cout << func.getName().str() << std::endl;
    }

    // auto &compiler_invocation = mCompilerInstance->getInvocation();
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
