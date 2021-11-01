#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service/Service.hpp>
#include <Usagi/Modules/Assets/SahProgramModule/SahProgramModule.hpp>
#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>
#include <Usagi/Modules/Runtime/Asset/ComponentSecondaryAssetRef.hpp>
#include <Usagi/Modules/Runtime/Asset/ServiceAssetManager.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/RuntimeModule.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/ServiceJitCompilation.hpp>

#include "ComponentCoroutineContinuation.hpp"

// #include <cxxabi.h>

// namespace __cxxabiv1
// {
// extern "C" {
//     extern  char *__cxa_demangle(const char *mangled_name,
//         char *output_buffer,
//         size_t *length, int *status);
// }
// }
//
// namespace abi = __cxxabiv1;


#include <llvm/Demangle/Demangle.h>

#include <memory>

namespace usagi
{
// template<typename T>
// std::string type_name()
// {
//     int status = 0;
//
//     std::unique_ptr<char, void(*)(void *)> res {
//         abi::__cxa_demangle(typeid(T).name(), NULL, NULL, &status),
//         std::free
//     };
//
//     if(status != 0) throw status; // stub
//
//     return res.get();
// }

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
                    // todo real parameters
                    auto handler = std::make_unique<SahProgramModule>(
                        jit,
                        // todo
                        "Database.pch", // db interface pch asset path
                        "Script.cpp"    // script source asset path
                    );
                    // append code for instantiating the script entry template


                    // std::stringstream ss, filtered;
                    // ss <<
                    auto demangled = llvm::demangle(typeid(decltype(db)).name());
                    // https://stackoverflow.com/a/20412841
                    auto replace_all = [](std::string &str, std::string_view s, std::string_view t) {
                        std::string::size_type n = 0;
                        while((n = str.find(s, n)) != std::string::npos)
                        {
                            str.replace(n, s.size(), t);
                            n += t.size();
                        }
                    };
                    replace_all(demangled, "class ", "");
                    replace_all(demangled, "struct ", "");
                    replace_all(demangled, "usagi::PagedStorageInMemory,1", "usagi::PagedStorageInMemory,usagi::entity::InsertionPolicy::REUSE_ARCHETYPE_PAGE");
                    // for(std::istream_iterator<std::string> i(ss), end; i != end; ++i)
                    // {
                    //     auto token = *i;
                    //     if(token != "class" && token != "struct")
                    //         filtered << token;
                    // }

                    auto instantiation = fmt::format(
                        R"(
extern "C"
{{
std::pair<std::uint64_t, ResumeCondition>
script_main(std::uint64_t entry, {} db)
{{
    return script(entry, db);
}}
}}
)", demangled);


                    std::cout << instantiation;

                    handler->set_additional(instantiation);

                    module_cache = asset_manager.secondary_asset(
                        std::move(handler),
                        work_queue
                    );
                    module_fp.fingerprint_build = module_cache.fingerprint_build;
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
                // getting the JIT-compiled function may involves code
                // generation which could affect the performance.
                auto script_coroutine =
                    module_bin->get_function_address<CoroutineT>(
                        "script_main"
                    );
                // todo provide heaps
                std::tie(script.next_entry_point, script.resume_condition) =
                    script_coroutine(script.next_entry_point, db);
            }
        }
    }
};
}
