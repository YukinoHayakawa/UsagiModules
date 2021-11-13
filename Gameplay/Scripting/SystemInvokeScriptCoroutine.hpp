#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service/Service.hpp>
#include <Usagi/Modules/Assets/SahProgramModule/SahProgramModule.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>
#include <Usagi/Modules/Runtime/Asset/ComponentSecondaryAssetRef.hpp>
#include <Usagi/Modules/Runtime/Asset/ServiceAssetManager.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/RuntimeModule.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/ServiceJitCompilation.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/Util.hpp>

#include "ComponentCoroutineContinuation.hpp"
#include "ComponentScript.hpp"
#include "ComponentScriptPCH.hpp"
// #include "ServiceDatabaseInterfaceProvider.hpp"

namespace usagi
{
struct SystemInvokeScriptCoroutine
{
    using WriteAccess = AllComponents;
    using ReadAccess = C<>;

    using ScriptFilterT = C<
        ComponentScript,
        ComponentCoroutineContinuation,
        ComponentSecondaryAssetRef
    >;

    void update(auto &&rt, auto &&db)
    {
        // todo: decide the execution order of scripts
        for(auto &&e : db.view(ScriptFilterT()))
            process_single(rt, db, e);
    }

    template <auto, auto>
    void process_single(auto &&rt, auto &&db, auto &&e)
    {
        auto &asset_manager = USAGI_SERVICE(rt, ServiceAssetManager);
        auto &jit = USAGI_SERVICE(rt, ServiceJitCompilation);
        auto &work_queue = USAGI_SERVICE(rt, ServiceAsyncWorker);
        // auto &heaps = USAGI_SERVICE(rt, ServiceHeapManager);
        // auto &executive = USAGI_SERVICE(rt, ServiceExecutive);
        auto &executive = USAGI_SERVICE(rt, ServiceStateTransitionGraph);
        // auto &dbi = USAGI_SERVICE(rt, ServiceDatabaseInterfaceProvider);

        static_assert(std::is_reference_v<decltype(db)>);

        using ModuleT = SahProgramModule::SecondaryAssetT;
        using CoroutineT =
            std::pair<
                std::uint64_t,
                ComponentCoroutineContinuation::ResumeCondition
            >(*)(std::uint64_t entry, decltype(db));

        auto &module_fp = USAGI_COMPONENT(e, ComponentSecondaryAssetRef);
        auto &state = USAGI_COMPONENT(e, ComponentCoroutineContinuation);
        auto &script = USAGI_COMPONENT(e, ComponentScript);

        SecondaryAssetMeta module_cache =
            asset_manager.secondary_asset(module_fp.fingerprint_build);

        if(const auto &status = module_cache.status;
            status == AssetStatus::READY)
        {
            // todo validate source.
            module_fp.fingerprint_dep_content =
                module_cache.fingerprint_dep_content;
        }
        else if(status == AssetStatus::MISSING_DEPENDENCY)
        {
            // depending on user options, the program may quit or this
            // event can be ignored at the cost of compromising the
            // correctness of computation
            // executive.report_critical_asset_missing_throw(module_fp);
            USAGI_THROW(std::runtime_error("")); // todo
        }
        // the module is not loaded into asset cache. try to load the
        // script source and compile it.
        else if(status == AssetStatus::MISSING)
        {
            // todo read from component
            auto handler = std::make_unique<SahProgramModule>(jit);

            // If the script specified a PCH to use, load it.
            if(e.template has_component<ComponentScriptPCH>())
            {
                auto &pch = USAGI_COMPONENT(e, ComponentScriptPCH);
                handler->set_pch(
                    pch.src_asset_path.to_string(),
                    pch.bin_asset_path.to_string()
                );
            }

            // Append main logic of the script.
            handler->add_asset_source(script.source_asset_path.to_string());

            // Append entry function for invoking the script.
            auto demangled = demangle_typeid(typeid(decltype(db)));
            auto instantiation = fmt::format(
                #include "EntrySourceTemplate.inc"
                , demangled
            );
            handler->add_string_source(
                fmt::format("script_entry/{}", script.source_asset_path),
                instantiation
            );
            LOG(debug, "Entry point source: \n{}", instantiation);

            module_cache = asset_manager.secondary_asset(
                std::move(handler),
                work_queue
            );
            module_fp.fingerprint_build = module_cache.fingerprint_build;
            return;
        }
        else if(status == AssetStatus::QUEUED || status == AssetStatus::LOADING)
        {
            // todo: capture the call stack
            // executive.drop_current_frame(ASSET_NOT_READY);
            return;
        }
        else USAGI_INVALID_ENUM_VALUE();

        bool resume;
        switch(state.resume_condition)
        {
            case ComponentCoroutineContinuation::NEVER:
            {
                resume = false;
                // todo remove
                executive.should_exit = true;
                break;
            }
            // todo
            // case ComponentCoroutineContinuation::USER_INPUT:
            // {
            //     auto &input = USAGI_SERVICE(rt, ServiceUserInput);
            //     resume = input.user_prompted_to_continue;
            //     break;
            // }
            case ComponentCoroutineContinuation::NEXT_FRAME:
            {
                resume = true;
                break;
            }
            default: USAGI_INVALID_ENUM_VALUE();
        }

        if(resume)
        {
            auto *module_bin = module_cache.asset->as<ModuleT>();
            auto entry_name = module_bin->search_function("script_main");
            USAGI_ASSERT_THROW(
                entry_name,
                std::runtime_error("Script has no entry point.")
            );
            // getting the JIT-compiled function may involves code
            // generation which could affect the performance.
            auto script_coroutine =
                module_bin->get_function_address<CoroutineT>(
                    entry_name.value()
                );
            // todo provide heaps
            std::tie(state.next_entry_point, state.resume_condition) =
                script_coroutine(state.next_entry_point, db);
        }
    }
};
}
