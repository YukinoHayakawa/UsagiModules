#pragma once

#include <Usagi/Module/Platform/WinCommon/Win32.hpp>

namespace usagi
{
struct RawInputSink
{
    static inline const wchar_t WINDOW_CLASS_NAME[] = L"UsagiWin32RawInputSink";
    HINSTANCE process_instance_handle = GetModuleHandleW(nullptr);
    HWND message_window = nullptr;

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
};
}
