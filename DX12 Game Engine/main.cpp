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
// Global structure stores general information about the main Win32 application.
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
void ThrowIfFailed(HRESULT);
BOOL CreateWindowHandle(int);
BOOL RegisterWndClass();
LRESULT CALLBACK WindowProcess(HWND, UINT, WPARAM, LPARAM);

//
// (Forward decl cont) DX12 Setup, helper functions, and draw function.
//
void CreateDevice();
void CreateCommandObjects();
void CreateSwapChain();
void CreateDescriptorHeaps();
void CreateFence();
void FlushCommandQueue();
void OnResize();
BOOL InitD3D12(); 

ID3D12Resource* CurrentBackBuffer();
D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView();
D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ID3D12Resource*, D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATES);

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
Microsoft::WRL::ComPtr<ID3D12Resource>              gDepthStencilBuffer;
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

    if (!RegisterWndClass())
        return EXIT_FAILURE;

    if (!CreateWindowHandle(commandShow))
        return EXIT_FAILURE;

    if (!InitD3D12())
        return EXIT_FAILURE;

    // Main message loop:
    MSG message = {};
    while (message.message != WM_QUIT)
    {
        if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        else
        {
            Draw();
        }
    }
    FlushCommandQueue();
    return static_cast<int>(message.wParam);
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

void CreateDevice() 
{
#if defined(DEBUG) || defined(_DEBUG)
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;

        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif
    
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&gDxgiFactory)));

    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr, // default hardware
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&gDevice));

    if (FAILED(hardwareResult)) // Use software rasterizer instead
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;

        ThrowIfFailed(
            gDxgiFactory->EnumWarpAdapter(
                IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(
            D3D12CreateDevice(
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&gDevice)));
    }

    gRtvDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    gDsvDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CreateCommandObjects() 
{
    // Command Queue
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};

    commandQueueDesc.Type       = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority   = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags      = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask   = 0;

    ThrowIfFailed(
        gDevice->CreateCommandQueue(
            &commandQueueDesc,
            IID_PPV_ARGS(&gCommandQueue)));

    // Command Allocator
    ThrowIfFailed(
        gDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&gCommandAllocator)));

    // Command List
    ThrowIfFailed(
        gDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            gCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&gCommandList)));

    // Command lists are created open. Close it so the first frame can Reset it.
    ThrowIfFailed(
        gCommandList->Close());
}

void CreateSwapChain() 
{
    // Describe buffer/swapchain display mode
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    constexpr DXGI_RATIONAL RefreshRate = {144, 1};

    // Display + refresh rate, & GPU memory formatting
    swapChainDesc.BufferDesc.Width              = g.Width;
    swapChainDesc.BufferDesc.Height             = g.Height;
    swapChainDesc.BufferDesc.RefreshRate        = RefreshRate;
    swapChainDesc.BufferDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering   = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling            = DXGI_MODE_SCALING_UNSPECIFIED;

    // TODO: Implement 4x (or variable... 2x, 4x, 8x, 16x...) MSAA
    // Multi-sampling
    swapChainDesc.SampleDesc.Count              = 1;
    swapChainDesc.SampleDesc.Quality            = 0;

    swapChainDesc.BufferUsage                   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount                   = SwapChainCount;
    swapChainDesc.OutputWindow                  = g.windowHandle;
    swapChainDesc.Windowed                      = true;
    swapChainDesc.SwapEffect                    = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags                         = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Create the swap chain
    ThrowIfFailed(
        gDxgiFactory->CreateSwapChain(
            gCommandQueue.Get(),
            &swapChainDesc,
            gSwapChain.GetAddressOf()));
}

void CreateDescriptorHeaps() 
{
    D3D12_DESCRIPTOR_HEAP_DESC renderTargetDesc;
    D3D12_DESCRIPTOR_HEAP_DESC depthStencilDesc;

    // Fill in render target description
    renderTargetDesc.NumDescriptors = SwapChainCount;
    renderTargetDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    renderTargetDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    renderTargetDesc.NodeMask       = 0;
    ThrowIfFailed(
        gDevice->CreateDescriptorHeap(
            &renderTargetDesc,
            IID_PPV_ARGS(&gRenderTargetViewHeap)
        )
    );

    // Fill in depth stencil description
    //depthStencilDesc.NumDescriptors = 1;
    //depthStencilDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    //depthStencilDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    //depthStencilDesc.NodeMask       = 0;
    //ThrowIfFailed(
    //    gDevice->CreateDescriptorHeap(
    //        &depthStencilDesc,
    //        IID_PPV_ARGS(&gDepthStencilViewHeap)
    //    )
    //);
}

void CreateFence() 
{
    ThrowIfFailed(gDevice->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&gFence)));
}

void FlushCommandQueue() 
{
    ++gCurrentFence;

    ThrowIfFailed(gCommandQueue->Signal(
        gFence.Get(),
        gCurrentFence));

    if (gFence->GetCompletedValue() < gCurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(
            nullptr,
            nullptr,
            false,
            EVENT_ALL_ACCESS);

        ThrowIfFailed(gFence->SetEventOnCompletion(
            gCurrentFence,
            eventHandle));

        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void OnResize() {
    assert(gDevice);
    assert(gSwapChain);
    assert(gCommandAllocator);

    // Flush before changing any resourceds
    FlushCommandQueue();

    ThrowIfFailed(gCommandList->Reset(gCommandAllocator.Get(), nullptr));

    // Release previous resources we will be creating
    for (int i = 0; i < SwapChainCount; ++i) {
        gSwapChainBuffers[i].Reset();
    }
    // TODO: left off here
    gDepthStencilBuffer.Reset();
}

BOOL InitD3D12() 
{
    try
    {
        CreateDevice();
        CreateCommandObjects();
        CreateSwapChain();
        CreateDescriptorHeaps();

        // TODO: Create RTV and DSV via an "OnResize" function
        OnResize();

        CreateFence();

        return true;
    }
    catch (const std::exception&)
    {
        MessageBox(nullptr, L"Direct3D 12 initialization failed.", L"Error", MB_OK);
        return false;
    }
}

ID3D12Resource* CurrentBackBuffer() 
{
    return gSwapChainBuffers[gCurrentBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() 
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        gRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

    handle.ptr += static_cast<SIZE_T>(gCurrentBackBufferIndex) * gRtvDescriptorSize;

    return handle;
}

D3D12_RESOURCE_BARRIER CreateTransitionBarrier(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES stateBefore,
    D3D12_RESOURCE_STATES stateAfter)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;

    return barrier;
}

void Draw() 
{
    ThrowIfFailed(gCommandAllocator->Reset());

    ThrowIfFailed(gCommandList->Reset(
        gCommandAllocator.Get(),
        nullptr));

    D3D12_RESOURCE_BARRIER barrierToRenderTarget =
        CreateTransitionBarrier(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

    gCommandList->ResourceBarrier(1, &barrierToRenderTarget);

    float clearColor[] = { 0.1f, 0.1f, 0.35f, 1.0f };

    // TODO: an exception is being caught here for some reason
    gCommandList->ClearRenderTargetView(
        CurrentBackBufferView(),
        clearColor,
        0,
        nullptr);

    D3D12_RESOURCE_BARRIER barrierToPresent =
        CreateTransitionBarrier(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);

    gCommandList->ResourceBarrier(1, &barrierToPresent);

    ThrowIfFailed(gCommandList->Close());

    ID3D12CommandList* commandLists[] = { gCommandList.Get() };

    gCommandQueue->ExecuteCommandLists(
        1,
        commandLists);

    ThrowIfFailed(gSwapChain->Present(1, 0));

    FlushCommandQueue();

    gCurrentBackBufferIndex =
        (gCurrentBackBufferIndex + 1) % SwapChainCount;
}
