#pragma once

#include <windows.h> // Windows header file must come first!
#include <debugapi.h>
#include <dxgi1_6.h>
#include <format>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>
#include <winerror.h>
#include <wrl.h>

#include "../core/comutils.h"

using namespace Microsoft::WRL;

// Public.
void LogAdapters(ComPtr<IDXGIFactory4> factory);
void ThrowIfFailed(HRESULT result, std::string message = "HRESULT failed.");

// Private.
const std::string _GetAdapterDesc(IDXGIAdapter* adapter);
const std::string _GetOutputDesc(IDXGIAdapter* adapter, DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM);
