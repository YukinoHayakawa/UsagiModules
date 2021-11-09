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

namespace usagi
{
struct SystemInvokeScriptCoroutine
{
    using WriteAccess = AllComponents;
    using ReadAccess = C<>;

    using ScriptFilterT = C<
        ComponentCoroutineContinuation,
        ComponentSecondaryAssetRef
    >;

    void update(auto &&rt, auto &&db)
    {
        auto &asset_manager = USAGI_SERVICE(rt, ServiceAssetManager);
        auto &jit = USAGI_SERVICE(rt, ServiceJitCompilation);
        auto &work_queue = USAGI_SERVICE(rt, ServiceAsyncWorker);
        // auto &heaps = USAGI_SERVICE(rt, ServiceHeapManager);
        // auto &executive = USAGI_SERVICE(rt, ServiceExecutive);
        auto &executive = USAGI_SERVICE(rt, ServiceStateTransitionGraph);

        using ModuleT = SahProgramModule::SecondaryAssetT;
        using CoroutineT =
            std::pair<
                std::uint64_t,
                ComponentCoroutineContinuation::ResumeCondition
            >(*)(std::uint64_t entry, decltype(db));

        // todo: decide the execution order of scripts
        for(auto &&e : db.view(ScriptFilterT()))
        {
            auto &module_fp = USAGI_COMPONENT(e, ComponentSecondaryAssetRef);
            auto &script = USAGI_COMPONENT(e, ComponentCoroutineContinuation);

            SecondaryAssetMeta module_cache =
                asset_manager.secondary_asset(module_fp.fingerprint_build);

            switch(module_cache.status)
            {
                case AssetStatus::READY:
                {
                    // todo validate source.
                    module_fp.fingerprint_dep_content = module_cache.fingerprint_dep_content;
                    break;
                }
                case AssetStatus::MISSING_DEPENDENCY:
                {
                    // depending on user options, the program may quit or this
                    // event can be ignored at the cost of compromising the
                    // correctness of computation
                    // executive.report_critical_asset_missing_throw(module_fp);
                    USAGI_THROW(std::runtime_error("")); // todo
                }
                // the module is not loaded into asset cache. try to load the
                // script source and compile it.
                case AssetStatus::MISSING:
                {
                    // todo read from component
                    auto handler = std::make_unique<SahProgramModule>(
                        jit,
                        "Database.i",
                        "Database.pch",
                        "Script.cpp"
                    );

                    // Append code for instantiating the script entry template
#ifdef _MSC_VER
                    auto demangled = demangle(typeid(decltype(db)).raw_name());
#else
                    USAGI_UNREACHABLE("unimplemented");
#endif
                    auto instantiation = fmt::format(
                        #include "EntrySourceTemplate.inc"
                        , demangled
                    );
                    LOG(debug, "Entry point source: \n{}", instantiation);
                    handler->set_additional_source_text(instantiation);

                    module_cache = asset_manager.secondary_asset(
                        std::move(handler),
                        work_queue
                    );
                    module_fp.fingerprint_build =
                        module_cache.fingerprint_build;
                    [[fallthrough]];
                }
                case AssetStatus::QUEUED: [[fallthrough]];
                case AssetStatus::LOADING:
                {
                    // todo: capture the call stack
                    // executive.drop_current_frame(ASSET_NOT_READY);
                    continue;
                }
                default: USAGI_UNREACHABLE("Incorrect asset state.");
            }

            bool resume;
            switch(script.resume_condition)
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
                assert(entry_name.has_value());
                // getting the JIT-compiled function may involves code
                // generation which could affect the performance.
                auto script_coroutine =
                    module_bin->get_function_address<CoroutineT>(
                        entry_name.value()
                    );
                // todo provide heaps
                std::tie(script.next_entry_point, script.resume_condition) =
                    script_coroutine(script.next_entry_point, db);
            }
        }
    }
};
}
