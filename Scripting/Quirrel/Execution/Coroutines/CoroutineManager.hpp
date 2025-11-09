#pragma once

#include <memory>
#include <vector>

#include "Coroutine.hpp"

namespace Sqrat
{
class Object;
} // namespace Sqrat

namespace usagi::scripting::quirrel
{
class VirtualMachine;

class CoroutineManager
{
public:
    explicit CoroutineManager(VirtualMachine & virtual_machine)
        : mVirtualMachine(virtual_machine)
    {
    }

    /**
     * @brief Main update tick. Resumes all coroutines and processes commands.
     */
    void tick_coroutines();

    /**
     * @brief Finds and creates coroutines from the main module's exports.
     */
    void _findAndCreateCoroutines(Sqrat::Object & exports);

    /**
     * @brief Shuts down and releases all active coroutines.
     * Used before a hot-reload or full shutdown.
     */
    void _shutdownAllCoroutines();

    /**
     * @brief Processes a gameplay command yielded from a coroutine.
     */
    void processCommand(
        const std::string_view & coroId, const std::string_view & command);

protected:
    VirtualMachine &       mVirtualMachine;
    std::vector<Coroutine> mActiveCoroutines;
};

} // namespace usagi::scripting::quirrel
