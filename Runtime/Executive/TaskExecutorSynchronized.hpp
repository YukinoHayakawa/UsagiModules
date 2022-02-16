#pragma once

#include <Usagi/Runtime/Task/TaskExecutor.hpp>

namespace usagi
{
class TaskExecutorSynchronized : public TaskExecutor
{
public:
    std::uint64_t submit(
        std::unique_ptr<Task> task,
        std::optional<std::vector<std::uint64_t>> wait_on) override;
};
}
