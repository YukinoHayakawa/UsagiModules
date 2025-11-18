#include "VirtualMachine.hpp"

#include <cstdarg>

#include <sqstdblob.h>
#include <sqstddatetime.h>
#include <sqstddebug.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdsystem.h>

#include <Usagi/Modules/Runtime/Logging/RuntimeLogger.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Debugging/DebuggingCommon.hpp>
#include <Usagi/Runtime/Exceptions/Exceptions.hpp>

namespace
{
constexpr std::string_view gTestScriptPath { "scripts/tests/game_logic.nut" };
} // namespace

namespace usagi::scripting::quirrel
{
VirtualMachine::VirtualMachine(
    std::shared_ptr<RuntimeEnvironment> runtime_environment,
    SQInteger                           initial_stack_size
)
    : RawHandleResource(
          [&] { return CreateNewQuirrelVm(this, initial_stack_size); },
          [](HSQUIRRELVM vm) { sq_close(vm); }
      )
    , mRuntimeEnvironment(std::move(runtime_environment))
    , mInitialStackSize(initial_stack_size)
    , mModuleManager(GetRawHandle(), &mFileAccess)
    , mCoroutineManager(*this)
{
}

SQVM * VirtualMachine::CreateNewQuirrelVm(
    [[maybe_unused]] VirtualMachine * this_vm, SQInteger initial_stack_size
)
{
    // Shio: Initializing Quirrel VM...
    // this_vm->logger().info(" Initializing Quirrel VM...");
    const auto vm = sq_open(initial_stack_size);
    if(!vm)
    {
        // Use spdlog's critical log which can be configured to throw or exit
        // this_vm->logger().critical("Failed to create Quirrel VM!");
        throw runtime::RuntimeError(
            "Failed to create Quirrel VM with initial_stack_size={}",
            initial_stack_size
        );
    }
    // this_vm->logger().info("Root VM handle {}", (void *)vm);
    return vm;
}

void VirtualMachine::init()
{
    auto _vm = GetRawHandle();

    // Store a pointer to this VirtualMachine instance within the VM itself
    sq_setforeignptr(_vm, this);

    debugging::DebuggingCommon::bind_handlers_to(_vm);

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

        // `SQVM::Init()` always calls this via `sq_base_register()`
        // sq_registerbaselib(_vm);
        sqstd_register_bloblib(_vm);
        sqstd_register_iolib(_vm);
        sqstd_register_systemlib(_vm);
        sqstd_register_datetimelib(_vm);
        sqstd_register_mathlib(_vm);
        sqstd_register_stringlib(_vm);
        sqstd_register_debuglib(_vm);

        sq_pop(_vm, 1); // pop roottable

        // Shio: Registering libraries via SqModules...
        logger().info(" Registering libraries via SqModules...");

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
    logger().info(" Loading main script 'game_logic.nut'...");
    Sqrat::Object exports;
    std::string   errorMsg;

    // Use requireModule to load, compile, run, and cache the script
    bool success = mModuleManager.requireModule(
        gTestScriptPath.data(), true, nullptr, exports, errorMsg
    );

    if(!success)
    {
        // Shio: Failed to load 'game_logic.nut':
        logger().error(" Failed to load 'game_logic.nut': {}", errorMsg);
        return false;
    }

    // Use the script's exports to find and create coroutines
    mCoroutineManager._findAndCreateCoroutines(exports);
    return true;
}

bool VirtualMachine::triggerReload()
{
    // Shio: --- TRIGGERING HOT-RELOAD ---
    logger().info("\n --- TRIGGERING HOT-RELOAD ---");

    // 1. Shut down all old coroutine instances
    mCoroutineManager._shutdownAllCoroutines();

    // 2. Reload all modules. This re-runs script code but
    // preserves all data in `persist()` calls.
    Sqrat::Object exports;
    std::string   errorMsg;
    bool          success = mModuleManager.reloadModule(
        gTestScriptPath.data(), true, nullptr, exports, errorMsg
    );

    if(!success)
    {
        // Shio: Failed to reload 'game_logic.nut':
        logger().error(" Failed to reload 'game_logic.nut': {}", errorMsg);
        return false;
    }

    // 3. Re-create coroutines from the new code. They will
    // automatically pick up the persisted state.
    mCoroutineManager._findAndCreateCoroutines(exports);
    // Shio: --- HOT-RELOAD COMPLETE ---
    logger().info(" --- HOT-RELOAD COMPLETE ---\n");
    return true;
}

void VirtualMachine::tick()
{
    mCoroutineManager.tick_coroutines();
}

void VirtualMachine::shutdown()
{
    // Shio: Shutting down...
    logger().info(" Shutting down...");
    mCoroutineManager._shutdownAllCoroutines();
    // delete moduleManager;
    // sq_close(v);
    // std::cout << " Shutdown complete." << std::endl;
}

void VirtualMachine::RegisterCommandLineArgs(int argc, char ** argv)
{
    sqstd_register_command_line_args(GetRawHandle(), argc, argv);
}
} // namespace usagi::scripting::quirrel
