// suppress llvm-related warnings
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wunused-variable"

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "CompilerInvocation.hpp"

#include <iostream>
#include <fstream>

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <clang/Basic/FileManager.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Serialization/InMemoryModuleCache.h>

#include <llvm/JITPDB/JITPDBMemoryManager.h>

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Runtime/File/RegularFile.hpp>

#include "CompilerFlagBuilder.hpp"
#include "RuntimeModule.hpp"

// Ref: https://blog.audio-tk.com/2018/09/18/compiling-c-code-in-memory-with-clang/

namespace usagi
{
namespace
{
class OutputToLogDiagnosticConsumer : public clang::TextDiagnosticPrinter
{
    llvm::SmallString<228> mOutStr;
    llvm::raw_svector_ostream mStream { mOutStr };

public:
    OutputToLogDiagnosticConsumer(clang::DiagnosticOptions *diags)
        : TextDiagnosticPrinter(mStream, diags, false)
    {
    }

    void HandleDiagnostic(
        const clang::DiagnosticsEngine::Level diag_level,
        const clang::Diagnostic &info) override
    {
        using namespace clang;
        using namespace logging;

        mOutStr.clear();
        TextDiagnosticPrinter::HandleDiagnostic(diag_level, info);

        LoggingLevel level = LoggingLevel::off;
        switch(diag_level)
        {
            // It's fatal for the compiler doesn't mean it's fatal for the
            // engine.
            case DiagnosticsEngine::Fatal:
            case DiagnosticsEngine::Error:
                level = LoggingLevel::error;
                break;
            case DiagnosticsEngine::Warning:
                level = LoggingLevel::warn;
                break;
            case DiagnosticsEngine::Remark:
            case DiagnosticsEngine::Note:
                level = LoggingLevel::info;
                break;
            case DiagnosticsEngine::Ignored:
            default: break;
        }

        LOG_V(level, "{}", mOutStr.c_str());
    }
};
}
void CompilerInvocation::create_diagnostics()
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &diagnostic_options = compiler_invocation.getDiagnosticOpts();
    diagnostic_options.ShowCategories = 2;
    diagnostic_options.ShowOptionNames = 1;

    auto text_diagnostic_printer =
        std::make_unique<OutputToLogDiagnosticConsumer>(&diagnostic_options);
    mCompilerInstance->createDiagnostics(
        text_diagnostic_printer.release(),
        true
    );
}

void CompilerInvocation::create_vfs()
{
    // Use a virtual filesystem to intercept the request of PCH source and
    // binary. Since PCH source always has a reference to its source in
    // an absolute path, we also have to find a platform-independent way
    // to fool clang when it wants the source. With clang, two types of
    // absolute path are handled. One is native filesystem path, which may
    // vary with system (e.g. on Windows it always starts with a drive name).
    // Therefore, even if we pass a remapping option to map the source of PCH
    // to root directory when compiling it, it will always be bound with
    // the drive name of the working directory of the compiler, which is not
    // what we want. Fortunately, clang also accepts network paths beginning
    // with "//net/", so we remap the PCH source to some name under that
    // directory and will be able to provide its source via our virtual
    // file system.
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

    std::stringstream opts = CompilerFlagBuilder::jit_options();
    LOG(info, "JIT options: {}", opts.str());

    llvm::SmallVector<std::string, 128> item_strs;
    llvm::SmallVector<const char *, 128> item_cstrs;

    for(std::istream_iterator<std::string> i(opts), end; i != end; ++i)
        item_strs.push_back(*i);

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
    )) USAGI_THROW(std::runtime_error("Failed to create compiler invocation."));

    // Prevent clang from trying to read from stdin.
    auto &front_end_options = compiler_invocation.getFrontendOpts();
    front_end_options.Inputs.clear();
}

void CompilerInvocation::add_virtual_file(
    std::string_view name,
    ReadonlyMemoryRegion bin)
{
    const llvm::StringRef ref_bin { bin.as_chars(), bin.length };
    // This won't copy the content of our memory region.
    auto buffer = llvm::MemoryBuffer::getMemBuffer(ref_bin, "", false);
    mFileSystem->addFile(name.data(), 0, std::move(buffer));
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
    // dtors of llvm objects invoked here.
}

CompilerInvocation & CompilerInvocation::set_pch(
    const ReadonlyMemoryRegion source,
    const ReadonlyMemoryRegion binary,
    std::optional<std::string> name)
{
    auto &compiler_invocation = mCompilerInstance->getInvocation();
    auto &preprocessor_options = compiler_invocation.getPreprocessorOpts();

    auto &pch_name = preprocessor_options.ImplicitPCHInclude;
    pch_name = name ? std::move(name.value()) : "<PCH>";

    // Add the source and binary of PCH to the vfs so clang can find it.
    // Also see create_vfs().
    add_virtual_file(fmt::format("//net/pch_source/{}", pch_name), source);
    add_virtual_file(pch_name, binary);

    return *this;
}

CompilerInvocation & CompilerInvocation::add_source(
    std::string_view name,
    ReadonlyMemoryRegion source)
{
    // Add a source location marker for our code section so we can pretend to
    // have multiple sources for our compilation unit :)
    fmt::vformat_to(
        std::back_insert_iterator(mSourceText),
        "# 1 \"{}\"\n",
        // ReSharper disable CppPossiblyUnintendedObjectSlicing
        fmt::make_format_args(name)
        // ReSharper restore CppPossiblyUnintendedObjectSlicing
    );
    mSourceText += source.to_string_view();
    return *this;
}

RuntimeModule CompilerInvocation::compile()
{
    USAGI_ASSERT_THROW(
        mCompilerInstance,
        std::logic_error("The compiler should not be reused.")
    );

    // Add source code.
    {
        auto &compiler_invocation = mCompilerInstance->getInvocation();
        auto &front_end_options = compiler_invocation.getFrontendOpts();
        llvm::MemoryBufferRef buffer { mSourceText, "source"};
        front_end_options.Inputs.push_back(
            clang::FrontendInputFile(buffer, { clang::Language::CXX })
        );
    }

    auto context = std::make_unique<llvm::LLVMContext>();
    auto action = std::make_unique<clang::EmitLLVMOnlyAction>(context.get());

    USAGI_ASSERT_THROW(
        mCompilerInstance->ExecuteAction(*action),
        std::runtime_error("Compilation failed.")
    );

    std::unique_ptr<llvm::Module> module = action->takeModule();

    USAGI_ASSERT_THROW(
        module,
        std::runtime_error("Unable to take IR module.")
    );

    // The global symbols are by default declared as COMDAT to allow the linker
    // to remove unused symbols. Doing so results in multiple .text/etc sections
    // which makes debugging harder. Solely removing COMDAT from the symbols
    // causes them to become weak, so manually set their linkages, too.
    auto remove_comdat = [](auto &&rng)
    {
        for(auto &o : rng)
        {
            o.setComdat(nullptr);
            o.setLinkage(llvm::GlobalValue::ExternalLinkage);
        }
    };
    remove_comdat(module->globals());
    remove_comdat(module->global_objects());
    remove_comdat(module->functions());

    llvm::PassBuilder pass_builder;
    llvm::LoopAnalysisManager loop_analysis_manager(false);
    llvm::FunctionAnalysisManager function_analysis_manager(false);
    llvm::CGSCCAnalysisManager cgscc_analysis_manager(false);
    llvm::ModuleAnalysisManager module_analysis_manager(false);

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

    // Do the optimization but preserve debuggability.
    llvm::ModulePassManager module_pass_manager =
        pass_builder.buildPerModuleDefaultPipeline(
            llvm::PassBuilder::OptimizationLevel::O1);
    module_pass_manager.run(*module, module_analysis_manager);

    // Create execution engine.

    // todo fix path. put in cache folder?
    auto memory_manager = std::make_unique<llvm::JITPDBMemoryManager>(
        "jit.pdb"
    );
    memory_manager->setVerbose(true);

    // doesn't work. visual studio simply doesn't read source from pdb
    // memory_manager->getPDBFileBuilder().addNatvisBuffer(
    //     "<source>",
    //     llvm::MemoryBuffer::getMemBufferCopy(mSource)
    // );

    const auto module_ptr = module.get();
    llvm::EngineBuilder builder(std::move(module));
    builder.setMCJITMemoryManager(
        // todo: diff config for debug/rel?
        // std::make_unique<llvm::SectionMemoryManager>()
        std::move(memory_manager)
    );
    builder.setOptLevel(llvm::CodeGenOpt::Level::None);
    builder.setEngineKind(llvm::EngineKind::JIT);

    std::unique_ptr<llvm::ExecutionEngine> engine(builder.create());
    USAGI_ASSERT_THROW(
        engine,
        std::runtime_error("Failed to build JIT engine.")
    );

    mCompilerInstance.reset();

    return { std::move(context), std::move(engine), module_ptr };
}
}
