#pragma once

#include <memory>

#include <sqmodules.h>
#include <squirrel.h>

#include <Usagi/Runtime/RAII/RawHandleResource.hpp>

#include "CoroutineManager.hpp"

namespace usagi::scripting::quirrel
{
class VirtualMachine
    : public RawHandleResource<HSQUIRRELVM>
    , public std::enable_shared_from_this<VirtualMachine>
{
public:
    // Shio: The constructor initializes the base RawHandleResource with
    // functions to create and destroy a Squirrel VM.
    // Shio: Initial stack size is 1024.
    explicit VirtualMachine(std::uint64_t initial_stack_size = 1'024);

    void RegisterCommandLineArgs(int argc, char ** argv);

    std::uint64_t initial_stack_size() const { return mInitialStackSize; }

    auto & module_manager(this auto &&self)
    {
        return self.mModuleManager;
    }

    auto & coroutine_manager(this auto &&self)
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
    std::uint64_t            mInitialStackSize;
    // Use smart pointers for automatic memory management
    DefSqModulesFileAccess   mFileAccess;
    SqModules                mModuleManager;
    // Track loaded scripts for hot reloading
    std::vector<std::string> mLoadedScripts;
    CoroutineManager         mCoroutineManager;

    static SQVM * CreateNewQuirrelVm(std::uint64_t initial_stack_size);

    void logLastError();
    static void printFunc(HSQUIRRELVM v, const SQChar * s, ...);
    static void errorFunc(HSQUIRRELVM v, const SQChar * s, ...);
    static void compileErrorHandler(
        HSQUIRRELVM       vm,
        SQMessageSeverity severity,
        const SQChar *    desc,
        const SQChar *    source,
        SQInteger         line,
        SQInteger         column,
        const SQChar *    extra_info);
    // Advanced debugging hook
    static void debugHook(
        HSQUIRRELVM,
        SQInteger      event_type,
        const SQChar * source_file,
        SQInteger      line,
        const SQChar * func_name);
    static SQInteger errorHandler(HSQUIRRELVM);
    void printCallstack();
};
} // namespace usagi::scripting::quirrel
