#include "../Win32.hpp"
#include <SetupAPI.h>
#pragma comment(lib, "SetupAPI.lib")
#include "../Win32MacroFix.hpp"

#include "Win32InputManager.hpp"

#include <Usagi/Core/Logging.hpp>
#include <Usagi/Utility/RawResource.hpp>
#include <Usagi/Utility/Unicode.hpp>

#include "Win32Mouse.hpp"
#include "Win32Keyboard.hpp"
#include "Win32Gamepad.hpp"
#include "../Win32Helper.hpp"

std::map<std::string, std::string> usagi::Win32InputManager::mDeviceNames;


// Some links:
// Querying kernel objects & following symbolic links:
// http://blogs.microsoft.co.il/pavely/2014/02/05/creating-a-winobj-like-tool/
// https://github.com/hfiref0x/WinObjEx64 (also where I get ntos.h)
// About undocumented Nt fuctions:
// http://undocumented.ntinternals.net/

// Source: Using SetupDi to enumerate devices
// http://samscode.blogspot.com/2009/08/setupdi-how-to-enumerate-devices-using.html
namespace
{
std::wstring getDeviceRegistryProperty(
    __in HDEVINFO hDevInfo,
    __in SP_DEVINFO_DATA DeviceInfoData,
    __in DWORD Property
)
{
    DWORD        data_t;
    std::wstring buffer;
    DWORD        buffer_size = 0;

    // Call function with null to begin with,
    // then use the returned buffer size (doubled)
    // to Alloc the buffer. Keep calling until
    // success or an unknown failure.
    //
    // Double the returned buffersize to correct
    // for underlying legacy CM functions that
    // return an incorrect buffersize value on
    // DBCS/MBCS systems.
    while(!SetupDiGetDeviceRegistryPropertyW(
        hDevInfo,
        &DeviceInfoData,
        Property,
        &data_t,
        reinterpret_cast<PBYTE>(buffer.data()),
        buffer_size,
        &buffer_size))
    {
        if(ERROR_INSUFFICIENT_BUFFER == GetLastError())
        {
            // Double the size to avoid problems on
            // W2k MBCS systems per KB 888609.
            buffer.resize(buffer_size * 2 / sizeof(std::wstring::value_type));
        }
        else
        {
            break;
        }
    }
    // Trim trailing zeros resulted from the oversized buffer.
    buffer.erase(std::find(buffer.begin(), buffer.end(), L'\0'), buffer.end());
    return buffer;
}
}

void usagi::Win32InputManager::updateDeviceNames()
{
    LOG(info, "Connected devices");
    LOG(info, "--------------------------------");

    // Create a HDEVINFO with all present devices.
    const USAGI_TRIVIAL_RAW_RESOURCE(HDEVINFO, dev_info, {
        dev_info = SetupDiGetClassDevsW(
            nullptr, // GUID
            nullptr, // Enumerator (HID?)
            nullptr,
            DIGCF_PRESENT | DIGCF_ALLCLASSES // find all connected devices
        );
        if(dev_info == INVALID_HANDLE_VALUE)
            USAGI_THROW(win32::Win32Exception("SetupDiGetClassDevs() failed."));
    }, {
        SetupDiDestroyDeviceInfoList(dev_info);
    });
    mDeviceNames.clear();

    // Enumerate through all devices in Set.
    SP_DEVINFO_DATA device_info_data;
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    for(DWORD i = 0; SetupDiEnumDeviceInfo(dev_info, i, &device_info_data); i++)
    {
        const auto device_obj = utf16To8(getDeviceRegistryProperty(
            dev_info, device_info_data, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME));

        if(device_obj.empty()) continue;

        auto name = utf16To8(getDeviceRegistryProperty(
            dev_info, device_info_data, SPDRP_FRIENDLYNAME));
        if(name.empty())
            name = utf16To8(getDeviceRegistryProperty(
                dev_info, device_info_data, SPDRP_DEVICEDESC));

        LOG(info, "{:24}: {}", device_obj, name);
        mDeviceNames.insert({ device_obj, name });
    }

    const auto last_error = GetLastError();
    if(NO_ERROR != last_error && ERROR_NO_MORE_ITEMS != last_error)
    {
        LOG(error, "Error occured while enumerating devices: {}",
            win32::getErrorMessage(last_error)
        );
    }
    LOG(info, "--------------------------------");
}

std::string usagi::Win32InputManager::deviceFriendlyName(
    const std::string &device_object_name)
{
    const auto iter = mDeviceNames.find(device_object_name);
    if(iter != mDeviceNames.end())
        return iter->second;
    return { };
}

void usagi::Win32InputManager::createVirtualDevices()
{
    mVirtualKeyboard = std::make_shared<Win32Keyboard>(
        nullptr, "Virtual Keyboard"
    );
    mVirtualMouse = std::make_shared<Win32Mouse>(
        nullptr, "Virtual Mouse"
    );
}

void usagi::Win32InputManager::fillRawInputBuffer(LPARAM lParam)
{
    UINT dwSize;

    // fetch raw input data
    GetRawInputData(
        reinterpret_cast<HRAWINPUT>(lParam),
        RID_INPUT,
        nullptr,
        &dwSize,
        sizeof(RAWINPUTHEADER)
    );
    mRawInputBuffer.resize(dwSize);
    if(GetRawInputData(
        reinterpret_cast<HRAWINPUT>(lParam),
        RID_INPUT,
        mRawInputBuffer.data(),
        &dwSize,
        sizeof(RAWINPUTHEADER)
    ) != dwSize)
    {
        USAGI_THROW(win32::Win32Exception(
            "GetRawInputData does not return correct size!"
        ));
    }
}

usagi::Win32InputManager::Win32InputManager()
{
    registerWindowClass();
    createInputSinkWindow();
    registerRawInputDevices();

    createVirtualDevices();
    updateDeviceNames();

    mDeviceEnumeration = Win32RawInputDevice::enumerateDevices();

    for(auto &&d : mDeviceEnumeration.keyboards)
        mRawInputDevices[d->deviceHandle()] = d;
    for(auto &&d : mDeviceEnumeration.mice)
        mRawInputDevices[d->deviceHandle()] = d;
    for(auto &&d : mDeviceEnumeration.gamepads)
        mRawInputDevices[d->deviceHandle()] = d;
}

usagi::Win32InputManager::~Win32InputManager()
{
    DestroyWindow(mMessageWindow);
    unregisterWindowClass();
}

LRESULT CALLBACK usagi::Win32InputManager::inputMessageDispatcher(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return instance()->inputMessageHandler(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK usagi::Win32InputManager::inputMessageHandler(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        // unbuffered raw input data
        case WM_INPUT:
        {
            fillRawInputBuffer(lParam);
            const auto raw = reinterpret_cast<RAWINPUT*>(
                mRawInputBuffer.data());

            // dispatch the message to virtual devices
            switch(raw->header.dwType)
            {
                case RIM_TYPEKEYBOARD:
                    mVirtualKeyboard->handleRawInput(raw);
                    break;

                case RIM_TYPEMOUSE:
                    mVirtualMouse->handleRawInput(raw);
                    break;

                default:
                    break;
            }

            // dispatch the message to specific device
            const auto iter = mRawInputDevices.find(raw->header.hDevice);
            if(iter != mRawInputDevices.end())
            {
                if(const auto dev = iter->second.lock())
                {
                    // ignore the message if the device is not used
                    if(dev.use_count() > 1)
                        dev->handleRawInput(raw);
                }
                else // the devices is removed
                {
                    mRawInputDevices.erase(iter);
                }
            }
            return 0;
        }
        // todo: handles device addition/removal
        case WM_INPUT_DEVICE_CHANGE:
        {

            break;
        }
        default:
            break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void usagi::Win32InputManager::processEvents()
{

}

std::shared_ptr<usagi::Keyboard> usagi::Win32InputManager::
    virtualKeyboard() const
{
    return mVirtualKeyboard;
}

std::shared_ptr<usagi::Mouse> usagi::Win32InputManager::virtualMouse() const
{
    return mVirtualMouse;
}

std::vector<std::shared_ptr<usagi::Gamepad>> usagi::Win32InputManager::
    gamepads() const
{
    std::vector<std::shared_ptr<Gamepad>> ret;
    ret.reserve(mDeviceEnumeration.gamepads.size());
    std::copy(
        mDeviceEnumeration.gamepads.begin(),
        mDeviceEnumeration.gamepads.end(),
        std::back_inserter(ret)
    );
    return ret;
}
