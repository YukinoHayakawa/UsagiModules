#pragma once

#include <vector>

#include <Usagi/Modules/Platforms/WinCommon/Win32.hpp>
#include <Usagi/Modules/Platforms/WinCommon/WindowMessageTarget.hpp>
#include <Usagi/Runtime/Memory/VmAllocatorPagefileBacked.hpp>

namespace usagi
{
struct RawInputSink : win32::WindowMessageTarget
{
    HINSTANCE process_instance_handle = GetModuleHandleW(nullptr);

    std::vector<std::byte> message_queue;

    void register_window_class();
    void create_input_sink_window();
    void register_raw_input_devices();

    void unregister_window_class();
    void destroy_input_sink_window();

    RawInputSink()
    {
        register_window_class();
        create_input_sink_window();
        register_raw_input_devices();
    }

    ~RawInputSink()
    {
        destroy_input_sink_window();
        unregister_window_class();
    }

    LRESULT message_handler(
        UINT message,
        WPARAM wParam,
        LPARAM lParam) override;
};
}
