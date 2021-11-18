#include "ServiceAsyncWorker.hpp"

#include <Usagi/Library/Memory/LockGuard.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Modules/Common/Time/Clock.hpp>
#include <Usagi/Runtime/Task.hpp>

namespace usagi
{
std::uint64_t StdTaskExecutor::submit(
    std::unique_ptr<Task> task,
    std::optional<std::vector<std::uint64_t>> wait_on)
{
    std::lock_guard lk(mMutex);

    auto task_id = ++mTaskId;

    LOG(trace, "[Executor] Submitting async task: {}", static_cast<void *>(task.get()));

    auto future = std::async(
        std::launch::async,
        [t = std::move(task), wait = std::move(wait_on), this, tid = task_id]() {
            Clock clk;
            // using namespace std::chrono_literals;
            // std::this_thread::sleep_for(5ms);

            LOG(trace, "[Executor] Preparing to execute task {} (worker thread={})", tid, std::this_thread::get_id());

            if(wait.has_value())
            {
                for(auto &&w : *wait)
                {
                    LockGuard lk(mMutex);
                    auto wt = mTask.find(w);
                    assert(wt != mTask.end());
                    lk.unlock();
                    LOG(trace, "Task {} waiting on {}", tid, wt->first);
                    wt->second.wait();
                }
            }

            if(!t->precondition())
                throw std::runtime_error("");
            t->on_started();
            LOG(trace, "[Executor] Executing task {} (worker thread={})", tid, std::this_thread::get_id());
            t->run();
            t->on_finished();
            if(!t->postcondition())
                throw std::runtime_error("");

            LOG(trace, "[Executor] Task {} consumed {} seconds.", tid, clk.realtime_elapsed());
        }
    );

    mTask.emplace(task_id, std::move(future));

    return task_id;
}
}
