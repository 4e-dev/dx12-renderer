#include "debug.h"

// 8/4/2026
// I spent way too long writing this...
// I have no idea if I'll ever even use this lmao.

// 8/5/2026
// Still working on this. I am an idiot, aren't I.
// Yes you are, love me from the day after...

void LogAdapters(ComPtr<IDXGIFactory4> factory) {
    // Reference: https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-enumadapters

    OutputDebugStringA("\n------------------\n");
    OutputDebugStringA("DEBUG::LogAdapters");
    OutputDebugStringA("\n------------------\n\n");

    std::vector<IDXGIAdapter*> adapters{};
    IDXGIAdapter* adapter{nullptr};
    for (UINT i{0}; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        std::string current_adapter_log{};
        current_adapter_log += std::format("ADAPTER({})", i);

        // Adapter description.
        current_adapter_log += "\n[DESCRIPTION]\n";
        current_adapter_log += _GetAdapterDesc(adapter);

        // Adapter outputs (connected displays).
        current_adapter_log += "[CONNECTED DISPLAY(s)]\n";
        current_adapter_log += _GetOutputDesc(adapter);
        current_adapter_log += "\n";

        OutputDebugStringA(current_adapter_log.c_str());
    }

    for (IDXGIAdapter* adapter : adapters) {
        ManuallyReleaseCom(adapter);
    }
}

void ThrowIfFailed(HRESULT result, std::string message) {
    if (FAILED(result)) {
        throw std::runtime_error(message);
    }
}

const std::string _GetAdapterDesc(IDXGIAdapter* adapter) {
    // Capture adapter description.
    DXGI_ADAPTER_DESC adapter_description{};
    adapter->GetDesc(&adapter_description);

    // Convert from WCHAR to CHAR via std::string.
    std::wstring long_description(adapter_description.Description);
    std::string description(long_description.begin(), long_description.end());

    // Print adapter description string.
    const std::string adapter_desc{ "\t" + description + "\n\n" };
    return adapter_desc;
}

const std::string _GetOutputDesc(IDXGIAdapter* adapter, DXGI_FORMAT format) {
    std::string text{};

    // Enumerate over adapter outputs (AKA displays connected to current adapter).
    IDXGIOutput* adapter_output{nullptr};
    UINT i;
    for (i = 0; adapter->EnumOutputs(i, &adapter_output) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_OUTPUT_DESC output_desc{};
        adapter_output->GetDesc(&output_desc);

        // Device name.
        std::wstring long_device_name(output_desc.DeviceName);
        std::string device_name(long_device_name.begin(), long_device_name.end());
        text += std::format("Name: ({})\n", device_name);

        // Resolution & refresh rate.
        UINT count{0}, flags{0};
        adapter_output->GetDisplayModeList(format, flags, &count, nullptr); // Call with `nullptr` to get list count.
        std::vector<DXGI_MODE_DESC> modes(count);
        adapter_output->GetDisplayModeList(format, flags, &count, &modes[0]);

        for (const DXGI_MODE_DESC mode : modes | std::views::reverse | std::views::take(3)) {
            UINT refresh_rate = mode.RefreshRate.Numerator;
            text += std::format("\tResolution: ({}, {}) {} hz\n", mode.Width, mode.Height, refresh_rate);
        }

        // Attached.
        bool attached = output_desc.AttachedToDesktop;
        text += std::format("\tAttached: {}\n", attached ? "True" : "False");
    }

    // Log information.
    if (i == 0) {
        return "No displays detected\n";
    }
    return text;
}
