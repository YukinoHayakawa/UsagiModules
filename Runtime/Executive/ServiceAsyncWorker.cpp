#include "ServiceAsyncWorker.hpp"

#include <cassert>
#include <iostream>

#include <Usagi/Library/Memory/LockGuard.hpp>
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

    auto future = std::async(
        std::launch::async,
        [t = std::move(task), wait = std::move(wait_on), this]() {
            Clock clk;
            // using namespace std::chrono_literals;
            // std::this_thread::sleep_for(5ms);

            if(wait.has_value())
            {
                for(auto &&w : *wait)
                {
                    LockGuard lk(mMutex);
                    auto wt = mTask.find(w);
                    assert(wt != mTask.end());
                    lk.unlock();
                    wt->second.wait();
                }
            }

            if(!t->precondition())
                throw std::runtime_error("");
            t->on_started();
            t->run();
            t->on_finished();
            if(!t->postcondition())
                throw std::runtime_error("");

            std::cout << clk.realtime_elapsed() << std::endl;
        }
    );

    mTask.emplace(task_id, std::move(future));

    return task_id;
}
}
