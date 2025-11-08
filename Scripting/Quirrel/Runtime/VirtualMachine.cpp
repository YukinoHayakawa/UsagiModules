#include "VirtualMachine.hpp"

#include <sqstdblob.h>
#include <sqstddatetime.h>
#include <sqstddebug.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdsystem.h>

#include <spdlog/spdlog.h>
// todo fix `<Windows.h>` included by `spdlog`
#undef GetObject

#include <Usagi/Runtime/Exceptions/Exceptions.hpp>

namespace
{
constexpr std::string_view gTestScriptPath { "scripts/tests/game_logic.nut" };
} // namespace

namespace usagi::scripting::quirrel
{
VirtualMachine::VirtualMachine(std::uint64_t initial_stack_size)
    : RawHandleResource(
          [&] { return CreateNewQuirrelVm(initial_stack_size); },
          [](HSQUIRRELVM vm) { sq_close(vm); })
    , mInitialStackSize(initial_stack_size)
    , mModuleManager(GetRawHandle(), &mFileAccess)
    , mCoroutineManager(*this)
{
}

SQVM *
VirtualMachine::CreateNewQuirrelVm(const std::uint64_t initial_stack_size)
{
    // Shio: Initializing Quirrel VM...
    spdlog::info(" Initializing Quirrel VM...");
    const auto vm = sq_open(initial_stack_size);
    if(!vm)
    {
        // Use spdlog's critical log which can be configured to throw or exit
        spdlog::critical("Failed to create Quirrel VM!");
        throw runtime::RuntimeError(
            "Failed to create Quirrel VM with initial_stack_size={}",
            initial_stack_size);
    }
    spdlog::info("Root VM handle {}", (void *)vm);
    return vm;
}

void VirtualMachine::init()
{
    auto _vm = GetRawHandle();

    // 1. Set handlers
    sq_setprintfunc(_vm, &printFunc, &errorFunc);
    // sq_setcompilererrorhandler(_vm, compileErrorHandler);

    // aux library
    // sets error handlers
    {
        sq_setcompilererrorhandler(_vm, &compileErrorHandler);
        // push new closure
        sq_newclosure(_vm, &errorHandler, 0);
        // set it as the new error handler
        sq_seterrorhandler(_vm);
        mModuleManager.compilationOptions.raiseError       = true;
        mModuleManager.compilationOptions.doStaticAnalysis = true;
    }
    sqstd_seterrorhandlers(_vm);

    // 2. Set up the debug hook for rich error reporting
    // Store a pointer to this VirtualMachine instance within the VM itself
    sq_setforeignptr(_vm, this);
    // todo fix
    // sq_setdebughook(_vm, debugHook);
    sq_setnativedebughook(_vm, &debugHook);

    // 3. Set up the module manager to handle all library registration
    // mFileAccess = std::make_unique<DefSqModulesFileAccess>();

    // This is the modern, centralized way to handle libraries.
    // mModuleManager = std::make_unique<SqModules>(_vm, mFileAccess.get());

    // Register modules
    // todo: limit functions for external file writes
    {
        // !!! MAKE SURE THAT THE STANDARD FUNCTIONS ARE REGISTERED WITH !!!
        // !!!  THE ROOT TABLE OTHERWISE THE SCRIPTS CANNOT ACCESS THEM  !!!
        // !!!     Quirrel IS ESPECIALLY STRICT ABOUT VARIABLE SCOPES    !!!
        sq_pushroottable(_vm);

        sqstd_register_bloblib(_vm);
        sqstd_register_iolib(_vm);
        sqstd_register_systemlib(_vm);
        sqstd_register_datetimelib(_vm);
        sqstd_register_mathlib(_vm);
        sqstd_register_stringlib(_vm);
        sqstd_register_debuglib(_vm);

        // sq_pop(_vm, 1); // pop roottable

        // Shio: Registering libraries via SqModules...
        spdlog::info(" Registering libraries via SqModules...");

        mModuleManager.registerMathLib();
        mModuleManager.registerStringLib();
        mModuleManager.registerSystemLib();
        mModuleManager.registerIoStreamLib();
        mModuleManager.registerIoLib();
        mModuleManager.registerDateTimeLib();
        mModuleManager.registerDebugLib();
    }
}

bool VirtualMachine::loadScripts()
{
    // Shio: Loading main script 'game_logic.nut'...
    spdlog::info(" Loading main script 'game_logic.nut'...");
    Sqrat::Object exports;
    std::string   errorMsg;

    // Use requireModule to load, compile, run, and cache the script
    bool success = mModuleManager.requireModule(
        gTestScriptPath.data(), true, nullptr, exports, errorMsg);

    if(!success)
    {
        // Shio: Failed to load 'game_logic.nut':
        spdlog::error(" Failed to load 'game_logic.nut': {}", errorMsg);
        return false;
    }

    // Use the script's exports to find and create coroutines
    mCoroutineManager._findAndCreateCoroutines(exports);
    return true;
}

bool VirtualMachine::triggerReload()
{
    // Shio: --- TRIGGERING HOT-RELOAD ---
    spdlog::info("\n --- TRIGGERING HOT-RELOAD ---");

    // 1. Shut down all old coroutine instances
    mCoroutineManager._shutdownAllCoroutines();

    // 2. Reload all modules. This re-runs script code but
    // preserves all data in `persist()` calls.
    Sqrat::Object exports;
    std::string   errorMsg;
    bool          success = mModuleManager.reloadModule(
        gTestScriptPath.data(), true, nullptr, exports, errorMsg);

    if(!success)
    {
        // Shio: Failed to reload 'game_logic.nut':
        spdlog::error(" Failed to reload 'game_logic.nut': {}", errorMsg);
        return false;
    }

    // 3. Re-create coroutines from the new code. They will
    // automatically pick up the persisted state.
    mCoroutineManager._findAndCreateCoroutines(exports);
    // Shio: --- HOT-RELOAD COMPLETE ---
    spdlog::info(" --- HOT-RELOAD COMPLETE ---\n");
    return true;
}

void VirtualMachine::tick()
{
    mCoroutineManager.tick_coroutines();
}

void VirtualMachine::shutdown()
{
    // Shio: Shutting down...
    spdlog::info(" Shutting down...");
    mCoroutineManager._shutdownAllCoroutines();
    // delete moduleManager;
    // sq_close(v);
    // std::cout << " Shutdown complete." << std::endl;
}

void VirtualMachine::RegisterCommandLineArgs(int argc, char ** argv)
{
    sqstd_register_command_line_args(GetRawHandle(), argc, argv);
}

// --- Logging and Debugging ---

void VirtualMachine::logLastError()
{
    auto _vm = GetRawHandle();

    const SQChar * err = nullptr;
    sq_getlasterror(_vm);
    if(SQ_SUCCEEDED(sq_getstring(_vm, -1, &err)) && err)
    {
        spdlog::error("[QUIRREL RUNTIME] {}", err);
    }
    sq_poptop(_vm); // Pop the error
}

void VirtualMachine::printFunc(HSQUIRRELVM, const SQChar * s, ...)
{
    char    buffer[1'024];
    va_list args;
    va_start(args, s);
    vsnprintf(buffer, sizeof(buffer), s, args);
    va_end(args);
    if(strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';
    spdlog::info("[QUIRREL] {}", buffer);
}

void VirtualMachine::errorFunc(HSQUIRRELVM, const SQChar * s, ...)
{
    char    buffer[1'024];
    va_list args;
    va_start(args, s);
    vsnprintf(buffer, sizeof(buffer), s, args);
    va_end(args);
    if(strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';
    spdlog::error("[QUIRREL ERR] {}", buffer);
}

void VirtualMachine::compileErrorHandler(
    HSQUIRRELVM,
    SQMessageSeverity s,
    const SQChar *    desc,
    const SQChar *    source,
    SQInteger         line,
    SQInteger         column,
    const SQChar *    extra)
{
    const char *              sev_str = "INFO ";
    spdlog::level::level_enum sev_lvl = spdlog::level::info;
    if(s == SEV_WARNING)
    {
        sev_str = "WARN ";
        sev_lvl = spdlog::level::warn;
    }
    if(s == SEV_ERROR)
    {
        sev_str = "ERROR";
        sev_lvl = spdlog::level::err;
    }
    spdlog::log(
        sev_lvl, "[{}] Compile Error in '{}' (line {}, col {}): {} {}", sev_str,
        source, line, column, desc, extra ? extra : "");
}

void VirtualMachine::debugHook(
    HSQUIRRELVM    v,
    SQInteger      event_type,
    const SQChar * source_file,
    SQInteger      line,
    const SQChar * func_name)
{
    auto fname   = func_name ? func_name : "unknown";
    auto srcfile = source_file ? source_file : "unknown";

    switch(event_type)
    {
        case 'l': // called every line(that contains some code)
            spdlog::debug(
                "LINE line [{0}] func [{1}] file [{2}]", line, fname, srcfile);
            break;
        case 'c': // called when a function has been called
            spdlog::debug(
                "LINE line [{0}] func [{1}] file [{2}]", line, fname, srcfile);
            break;
        case 'r': // called when a function returns
            spdlog::debug(
                "LINE line [{0}] func [{1}] file [{2}]", line, fname, srcfile);
            break;
    }

    errorHandler(v);
}

SQInteger VirtualMachine::errorHandler(HSQUIRRELVM v)
{
    // SQInteger event_type;
    // sq_getinteger(v, 2, &event_type);

    // todo: this is no such an `event_type`
    // if(event_type == 'e') // 'e' for "exception thrown"
    {
        VirtualMachine * vmInstance = (VirtualMachine *)sq_getforeignptr(v);
        if(vmInstance)
        {
            spdlog::critical("--- QUIRREL RUNTIME ERROR ---");
            vmInstance->printCallstack();
        }
    }

    return 0;
}

// todo: see `sqstd_printcallstack`/`_sqstd_aux_printerror`
void VirtualMachine::printCallstack()
{
    SQStackInfos si;
    SQInteger    level = 1;
    while(SQ_SUCCEEDED(sq_stackinfos(GetRawHandle(), level, &si)))
    {
        const char * func = si.funcname ? si.funcname : "unknown_function";
        const char * src  = si.source ? si.source : "unknown_source";
        spdlog::error("  [{}] {}() [{}:{}]", level, func, src, si.line);
        level++;
    }
}
} // namespace usagi::scripting::quirrel
