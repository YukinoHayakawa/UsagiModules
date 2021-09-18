#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service.hpp>

#include "ComponentCoroutineContinuation.hpp"

namespace usagi
{
struct SystemInvokeScriptCoroutine
{
    using WriteAccess = AllComponents;
    using ReadAccess = C<>;

    using ScriptFilterT = C<
        ComponentCoroutineContinuation
    >;

    void update(auto &&rt, auto &&db)
    {
        auto &asset = USAGI_SERVICE(rt, ServiceAssetManager);
        auto &modules = USAGI_SERVICE(rt, ServiceProgramModule);
        auto &toolchain = USAGI_SERVICE(rt, ServiceCompilerToolchain);
        auto &heaps = USAGI_SERVICE(rt, ServiceHeapManager);
        auto &executive = USAGI_SERVICE(rt, ServiceExecutive);

        using CoroutineT = std::uint64_t(*)(std::uint64_t entry, decltype(db));

        // todo: decide the execution order of scripts
        for(auto &&e : db.view(ScriptFilterT()))
        {
            const auto &c_asset = USAGI_COMPONENT(e, ComponentAssetMetadata);
            auto &c_script = USAGI_COMPONENT(e, ComponentCoroutineContinuation);
            auto name = build_module_name(c_asset.name);

            auto asset_agent = asset.find(name);
            const auto status = asset_agent.status();

            // make sure the script is loaded
            switch(status)
            {
                // the asset couldn't be found in any source
                case MISSING:
                {
                    // depending on user options, the program may quit or
                    // it can be ignored at the cost of compromising the
                    // correctness of computation
                    executive.report_critical_asset_missing_throw(c_asset);
                    continue;
                }
                // the asset exists, but not loaded
                case FOUND:
                {
                    asset_agent.load();
                    goto drop_frame;
                }
                // the raw asset is cached in the memory for use
                case READY:
                {
                    asset_agent.postprocess_bytestream([&](auto &bytestream) {
                        auto compiler = toolchain.compiler();
                        compiler.set_pch(asset.find(/* pch */));
                        compiler.inject_header(/* db interface pch */);
                        compiler.append_source(/* script source */);
                        compiler.append_source(/* template function instantiation */);
                        auto result = compiler.compile();
                        result.report_errors();
                        return std::move(result.bytestream());
                    });
                    goto drop_frame;
                }
                // the asset has been postprocessed and the product is ready
                // for use
                case POSTPROCESSED_READY:
                {
                    // loading dynamic library is an blocking action as in
                    // program initialization
                    auto module_agent = modules.find(name);
                    if(!module_agent.loaded())
                        module_agent.load(asset_agent.postprocessed_bytestream());
                    // postcondition: script module loaded
                    break;
                }
                default:
                {
                    assert(asset_agent.pending());
                drop_frame:
                    // todo: capture the callstack
                    executive.drop_current_frame(ASSET_NOT_READY);
                    continue;
                }
            }

            bool resume;
            switch(c_script.resume_condition)
            {
                case ComponentCoroutineContinuation::NEVER:
                {
                    resume = false;
                    break;
                }
                case ComponentCoroutineContinuation::USER_INPUT:
                {
                    auto &input = USAGI_SERVICE(rt, ServiceUserInput);
                    resume = input.user_prompted_to_continue;
                    break;
                }
                case ComponentCoroutineContinuation::NEXT_FRAME:
                {
                    resume = true;
                    break;
                }
                default: USAGI_UNREACHABLE();
            }
            if(resume)
            {
                auto script_module = modules.find(name);
                auto script_coroutine = script_module.proc<CoroutineT>("script");
                // todo provide heaps
                std::tie(c_script.next_entry_point, c_script.resume_condition) =
                    script_coroutine(c_script.next_entry_point, db);
            }
        }
    }
};
}
