#pragma once

#include <future>
#include <map>

#include <Usagi/Runtime/Task/TaskExecutor.hpp>
#include <Usagi/Runtime/Service/SimpleService.hpp>

namespace usagi
{
// todo: this is a temporary replacement
class StdTaskExecutor : public TaskExecutor
{
    std::map<std::uint64_t, std::future<void>> mTask;
    using TaskRef = decltype(mTask)::iterator;
    std::mutex mMutex;
    std::uint64_t mTaskId = 0;

public:
    std::uint64_t submit(
        std::unique_ptr<Task> task,
        std::optional<std::vector<std::uint64_t>> wait_on) override;
};

using ServiceAsyncWorker = SimpleService<StdTaskExecutor>;
}
