#pragma once

#include <memory>

#include <sqmodules.h>
#include <squirrel.h>

#include <Usagi/Modules/Scripting/Quirrel/Debugging/DebuggingInterface.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/Coroutines/CoroutineManager.hpp>
#include <Usagi/Modules/Scripting/Quirrel/Execution/Execution.hpp>
#include <Usagi/Modules/Scripting/Quirrel/RuntimeEnvironment.hpp>

namespace usagi::runtime
{
class RuntimeLogger;
} // namespace usagi::runtime

namespace usagi::scripting::quirrel
{
class VirtualMachine
    : public runtime::RawHandleResource<ExecutionContextHandle>
    , public debugging::DebuggingInterface
    , public std::enable_shared_from_this<VirtualMachine>
{
public:
    // Shio: The constructor initializes the base RawHandleResource with
    // functions to create and destroy a Squirrel VM.
    VirtualMachine(
        std::shared_ptr<RuntimeEnvironment> runtime_env,
        SQInteger                           stack_size_override = -1
    );

    HSQUIRRELVM get_vm() const { return GetRawHandle(); }

    auto & logger() const
    {
        return mRuntimeEnvironment->service_provider
            .ensure_service<runtime::RuntimeLogger>();
    }

    void RegisterCommandLineArgs(int argc, char ** argv);

    SQInteger initial_stack_size() const { return mInitialStackSize; }

    auto & module_manager(this auto && self) { return self.mModuleManager; }

    auto & coroutine_manager(this auto && self)
    {
        return self.mCoroutineManager;
    }

    virtual void init();

    /**
     * @brief Loads the main entry-point script and creates coroutines.
     */
    bool loadScripts();

    /**
     * @brief Triggers a stateful hot-reload of all scripts.
     */
    bool triggerReload();

    /**
     * @brief Main update tick. Resumes all coroutines and processes commands.
     */
    void tick();

    /**
     * @brief Shuts down the entire server.
     */
    void shutdown();

protected:
    /*
    struct RuntimeEnvironment;

    consteval
    {
        std::meta::define_aggregate(
            ^^RuntimeEnvironment,
            { std::meta::data_member_spec(
                ^^SimpleServiceProvider *, { .name = "service_provider" }
            ) }
        );
    }
    */

    std::shared_ptr<RuntimeEnvironment> mRuntimeEnvironment;
    SQInteger                           mInitialStackSize;
    // Use smart pointers for automatic memory management
    DefSqModulesFileAccess              mFileAccess;
    SqModules                           mModuleManager;
    // Track loaded scripts for hot reloading
    std::vector<std::string>            mLoadedScripts;
    CoroutineManager                    mCoroutineManager;

    static SQVM * CreateNewQuirrelVm(
        VirtualMachine * this_vm, SQInteger initial_stack_size
    );

    ExecutionContextHandle get_self_vm_impl() const override
    {
        return GetRawHandle();
    }

    RuntimeEnvironment & get_runtime_environment() const override
    {
        return *mRuntimeEnvironment;
    }
};
} // namespace usagi::scripting::quirrel
