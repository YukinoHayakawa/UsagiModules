#pragma once

#include <thread>

#include <taskflow/taskflow.hpp>

#include <Usagi/Entity/System.hpp>
#include <Usagi/Entity/detail/EntityDatabaseAccessExternal.hpp>

namespace usagi
{
template <System... EnabledSystems>
class ExecutorTaskflow
{
    tf::Executor mExecutor;
    tf::Taskflow mTaskflow;

    template <System Sys>
    void foo(Sys &sys)
    {
        auto runtime = ;
        // this is a database view of the original database with modifications
        // made by the systems preceding the current one in some order.
        auto db = EntityDatabaseAccessExternal<
            Database,
            ComponentAccessSystemAttribute<Sys>
        >(&db);
        // todo memory mgmt
        std::vector<std::unique_ptr<typename Sys::ChunkContext>> chunk_ctx;

        // create fence task
        auto sys_task = mTaskflow.emplace([
            // todo memory mgmt
            sys_ctx = std::make_unique<typename Sys::SystemContext>(
                db, runtime
            ),
            &runtime,
            db,
            &chunk_ctx
        ](tf::Subflow &subflow) {
            auto begin = db.begin();
            auto end = db.end();

            // https://codereview.stackexchange.com/a/106837
            const auto num_pages = std::distance(begin, end);
            const auto num_chunks = std::max(
                1u, std::thread::hardware_concurrency() * 2
            );

            chunk_ctx.reserve(num_chunks);

            // create chunk contexts

            const auto chunk_size = num_pages / num_chunks;
            const auto chunk_remainder = num_pages % num_chunks;

            // create chunked tasks
            //
            // these tasks finally join into the main task. since the main task
            // is only executed when its preceding tasks are finished, the
            // precedence constraints are properly maintained.
            while(begin != end)
            {
                auto chunk_begin = begin;
                auto chunk_end = std::next(
                    begin,
                    chunk_size + chunk_remainder ? 1 : 0
                );
                begin = chunk_end;
                if(chunk_remainder) --chunk_remainder;

                // todo memory mgmt
                chunk_ctx.emplace_back(
                    std::make_unique<typename Sys::ChunkContext>(
                        sys_ctx, db, runtime
                    )
                );

                subflow.emplace([
                    chunk_begin, chunk_end,
                    db,
                    chunk_ctx = chunk_ctx.back().get()
                ]() mutable {
                    for(; chunk_begin != chunk_end; ++chunk_begin)
                    {
                        auto &&p = *chunk_begin;
                        auto entity = db.page_view(p, Sys::Filter());
                        auto entity_begin = entity.begin();
                        auto entity_end = entity.end();
                        for(; entity_begin != entity_end; ++entity_begin)
                        {
                            // if any entity was created during the
                            Sys::update(chunk_ctx, *entity_begin);
                        }
                    }
                });
            }
        }).name(typeid(Sys).name());

        // create database merge task
        auto merge_task = mTaskflow.emplace([]() {

        });

        sys_task.precede(merge_task);
    }
};
}
