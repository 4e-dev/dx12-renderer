#include "framework.h"
#include "resource.h"

#include <cstdlib>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <stdexcept>
#include <string>
#include <windows.h>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

//
// Global structure stores generael information about the main Win32 application.
//
struct Global {
    HINSTANCE instance  = nullptr;
    HWND windowHandle   = nullptr;
    INT Height          = 1920;
    INT Width           = 1080;
    WCHAR* title;
    WCHAR* windowClass;
};

Global g = {};

//
// Other global variables
//
UINT gCurrentBackBufferIndex    = 0;
UINT gDsvDescriptorSize         = 0;
UINT gRtvDescriptorSize         = 0;
UINT64 gCurrentFence            = 0;
constexpr INT SwapChainCount    = 2;

//
// Forward declarations
//
BOOL CreateWindowHandle(int commandShow);
BOOL RegisterWndClass();
LRESULT CALLBACK WindowProcess(HWND, UINT, WPARAM, LPARAM);
void ThrowIfFailed(HRESULT result);

//
// (Forward decl cont) DX12 Setup, helper functions, and draw function.
//
void CreateDevice();
void CreateCommandObjects();
void CreateSwapChain();
void CreateDescriptorHeaps();
void CreateFence();
void FlushCommandQueue();
BOOL InitD3D12(); 

ID3D12Resource* CurrentBackBuffer();
D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView();
D3D12_RESOURCE_BARRIER CreateTransitionBarrier();

void Draw();

//
// COM pointers for D3D12 and DXGI
//
Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      gCommandAllocator;
Microsoft::WRL::ComPtr<ID3D12CommandQueue>          gCommandQueue;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        gDepthStencilViewHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        gRenderTargetViewHeap;
Microsoft::WRL::ComPtr<ID3D12Device>                gDevice;
Microsoft::WRL::ComPtr<ID3D12Fence>                 gFence;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   gCommandList;
Microsoft::WRL::ComPtr<ID3D12Resource>              gSwapChainBuffers[SwapChainCount];
Microsoft::WRL::ComPtr<IDXGIFactory4>               gDxgiFactory;
Microsoft::WRL::ComPtr<IDXGISwapChain>              gSwapChain;

//
// App entry point
//
int APIENTRY wWinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE previousInstance,
    _In_        LPWSTR commandLine,
    _In_        int commandShow)
{
    // Register global variables
    g.instance = hInstance;
    g.title = (WCHAR*) L"Asset Loader";
    g.windowClass = (WCHAR*) L"MainWindow";

    // Register a new window
    if (!RegisterWndClass())
        return EXIT_FAILURE;

    // Create the main window
    if (!CreateWindowHandle(commandShow))
        return EXIT_FAILURE;

    // Main message loop:
    MSG message;
    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return (int) message.wParam;
}

void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result))
    {
        throw std::runtime_error("HRESULT failed");
    }
}

BOOL RegisterWndClass() {
    WNDCLASSEXW wndClass = {
        sizeof(WNDCLASSEX),
        CS_VREDRAW | CS_HREDRAW,
        WindowProcess,
        0,
        0,
        g.instance,
        LoadIcon(nullptr, IDI_APPLICATION),
        LoadCursor(nullptr, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW+1),
        NULL,
        g.windowClass,
        NULL};

    if (RegisterClassExW(&wndClass) == 0)
        return false;

    return true;
}

BOOL CreateWindowHandle(const int commandShow)
{
    HWND windowHandle = CreateWindowW(
        g.windowClass,
        g.title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        nullptr,
        nullptr,
        g.instance,
        nullptr);

    if (!windowHandle)
        return false;

    g.windowHandle = windowHandle;
    ShowWindow(g.windowHandle, commandShow);
    UpdateWindow(g.windowHandle);

    return true;
}

LRESULT CALLBACK WindowProcess(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC deviceContextHandle = BeginPaint(windowHandle, &ps);
        // TODO: Add any drawing code that uses hdc here...
        // ...
        EndPaint(windowHandle, &ps);
    }
    else if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
    }
    else if (message == WM_KEYDOWN)
    {
        if (wParam == VK_ESCAPE) // TODO: temporary; escape will pause the game
        {
            PostQuitMessage(0);
        }
    }

	return DefWindowProc(windowHandle, message, wParam, lParam);
}
