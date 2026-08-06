#include "./app.h"
#include "debug/debug.h"

#define SWAP_CHAIN_BUFFER_COUNT 2
#define DEPTH_STENCIL_BUFFER_COUNT 1

using namespace Microsoft::WRL;

namespace DX12 {
    ComPtr<ID3D12CommandAllocator> CommandAllocator;
    ComPtr<ID3D12CommandQueue> CommandQueue;
    ComPtr<ID3D12Device> Device; // The device owns all GPU resources.
    ComPtr<ID3D12Fence> Fence;
    ComPtr<ID3D12GraphicsCommandList> CommandList;
    ComPtr<IDXGIFactory4> Factory;
}

namespace Descriptor {
    UINT CbvSrvUavSize = 0;
    UINT DsvSize = 0;
    UINT RtvSize = 0;
}

namespace Frame {
    UINT CurrentBackBuffer = 0;
    UINT64 CurrentFence = 0;
}

namespace Graphics {
    D3D12_RECT ScissorRect = {};
    D3D12_VIEWPORT Viewport = {};
    DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
}

namespace RenderTarget {
    ComPtr<ID3D12DescriptorHeap> DsvHeap;
    ComPtr<ID3D12DescriptorHeap> RtvHeap;
    ComPtr<ID3D12Resource> DepthStencilBuffer;
}

namespace SwapChain {
    ComPtr<ID3D12Resource> Buffers[SWAP_CHAIN_BUFFER_COUNT];
    ComPtr<IDXGISwapChain> Chain;
}

namespace Window {
    HINSTANCE Instance = nullptr;
    HWND Handle = nullptr;
    bool Minimized = false;
    bool Paused = false;
    int Height = 1080;
    int Width = 1920;
}

//
// Function declarations
//
D3D12_RESOURCE_BARRIER TransitionBarrier(
    ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

LRESULT CALLBACK WindowProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND CreateWindowHandle();
void CheckMsaaSupport();
void CreateCommandObjects();
void CreateDescriptorHeaps();
void CreateDevice(const D3D_FEATURE_LEVEL minimum_feature_level);
void CreateFence();
void CreateSwapChain();
void Draw();
void GetDescriptorSizes();
void OnResize();
void FlushQueue();

//
// Entry point
//
int APIENTRY wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPWSTR,
    int commandShow)
{
    Window::Instance = instance;

    //
    // Create a Window (win32).
    //
    Window::Handle = CreateWindowHandle();
    ShowWindow(Window::Handle, commandShow);
    UpdateWindow(Window::Handle);

    // Create the DX12 Factory.
    ThrowIfFailed(
        CreateDXGIFactory1(IID_PPV_ARGS(&DX12::Factory)));

    #if defined(DEBUG) || defined(_DEBUG)
        LogAdapters(DX12::Factory);
    #endif
 
    //
    // Initialize D3D12
    //
    try {
        CreateDevice(D3D_FEATURE_LEVEL_11_0);
        CreateFence();
        GetDescriptorSizes();
        CheckMsaaSupport();
        CreateCommandObjects();
        CreateSwapChain();
        CreateDescriptorHeaps();
        OnResize();
    } catch (...) {
        MessageBoxA(
            nullptr,
            "Failed to initialize D3D12.",
            "Error",
            MB_OK | MB_ICONERROR);

        return EXIT_FAILURE;
    }

    //
    // MAIN LOOP
    //
    MSG message = {0};
    while (message.message != WM_QUIT) {
        // Process messages
        if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
            continue;
        }
        // Render game
        else if (!Window::Paused) {
            Draw();
        }
    }
    FlushQueue();
    return static_cast<int>(message.wParam);
}

//
// Window procedure
//
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE: { // Pressing the 'X' button
        DestroyWindow(hWnd); // Sends a WM_DESTROY message.
        break;
    }
    case WM_DESTROY: { // Destroy window
        PostQuitMessage(0); // Calls WM_QUIT
        break;
    }
    case WM_KEYDOWN: { // Key being pressed
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        return 0;
    }
    case WM_SIZE: { // Window is resized.
        Window::Width = LOWORD(lParam);
        Window::Height = HIWORD(lParam);

        if (DX12::Device) {
            // Do not render when minimized.
            if (wParam == SIZE_MINIMIZED) {
                Window::Minimized = true;
                Window::Paused = true;
            }
            else if (wParam == SIZE_MAXIMIZED) {
                Window::Minimized = false;
                Window::Paused = false;
                OnResize();
            }
            else if (wParam == SIZE_RESTORED) {
                // The ordinary window state (non-maximized, non-minimized).
                if (Window::Minimized) {
                    Window::Minimized = false;
                    Window::Paused = false;
                    OnResize();
                }
            }
        }
    }}
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//
// Creates a win32 window class and registers it.
//
HWND CreateWindowHandle() {
    //
    // Register the window class.
    //
    WNDCLASSEXW windowClass = {};

    const std::wstring className = L"D3D12WindowClass";
    const std::wstring windowName = L"D3D12 Renderer"; 

    windowClass.cbSize           = sizeof(WNDCLASSEXW);
    windowClass.hCursor          = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hIcon            = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hInstance        = Window::Instance;
    windowClass.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpfnWndProc      = WindowProc;
    windowClass.lpszClassName    = className.c_str();
    windowClass.style            = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExW(&windowClass)) {
        std::println("CreateWindowHandle()::Unable to register window class");
        return nullptr;
    }

    //
    // Create window
    //
    HWND windowHandle = CreateWindowW(
        className.c_str(),
        windowName.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        Window::Width,
        Window::Height,
        nullptr, // This window won't be owned by anyone else.
        nullptr, // It also does not own a child window.
        Window::Instance,
        nullptr // Additional data not needed.
    );

    if (!windowHandle) {
        std::println("CreateWindowHandle()::Unable to create window handle");
        return nullptr;
    }

    return windowHandle;
}

//
// Device
//
void CreateDevice(const D3D_FEATURE_LEVEL minimum_feature_level) {

    #if defined(DEBUG) || defined(_DEBUG)
        {
        Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
            debug_controller->EnableDebugLayer();
        }
    #endif

    HRESULT hardware_result = D3D12CreateDevice(
        nullptr,
        minimum_feature_level,
        IID_PPV_ARGS(&DX12::Device));

    // Fall back to WARP (software rasterizer)
    if (FAILED(hardware_result)) {
        Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
        ThrowIfFailed(
            DX12::Factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)));

        ThrowIfFailed(
            D3D12CreateDevice(
                warp_adapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&DX12::Device)));
    }
}

void CreateFence() {
    ThrowIfFailed(
        DX12::Device->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&DX12::Fence)));
}

void GetDescriptorSizes() {
    Descriptor::RtvSize =
        DX12::Device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    Descriptor::DsvSize =
        DX12::Device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    Descriptor::CbvSrvUavSize =
        DX12::Device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CheckMsaaSupport() {
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQuality = {};

    // NOTE(bao): Eventually, I want the user to be able to select
    // their desired MSAA (multi-sampling) quality.
    // For now, we `disable` MSAA by setting SampleCount to 1 and NumQualityLevels to 0...
    msaaQuality.Format              = Graphics::BackBufferFormat;
    msaaQuality.SampleCount         = 1;
    msaaQuality.Flags               = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaQuality.NumQualityLevels    = 0;

    ThrowIfFailed(
        DX12::Device->CheckFeatureSupport(
            D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &msaaQuality,
            sizeof(msaaQuality)));
}

//
// Command objects
//
void CreateCommandObjects() {
    // Create command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(
        DX12::Device->CreateCommandQueue(
            &queueDesc,
            IID_PPV_ARGS(&DX12::CommandQueue)));

    // Create command allocator.
    ThrowIfFailed(
        DX12::Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&DX12::CommandAllocator)));

    // Create command list.
    ThrowIfFailed(
        DX12::Device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            DX12::CommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&DX12::CommandList)));

    // To prevent accidental command recording, close immediately.
    ThrowIfFailed(
        DX12::CommandList->Close());
}

//
// Swap chain
//
void CreateSwapChain() {
    SwapChain::Chain.Reset();

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    // Buffer description
    swapChainDesc.BufferDesc.Width                      = Window::Width;
    swapChainDesc.BufferDesc.Height                     = Window::Height;
    swapChainDesc.BufferDesc.RefreshRate.Numerator      = 120;
    swapChainDesc.BufferDesc.RefreshRate.Denominator    = 1;
    swapChainDesc.BufferDesc.Format                     = Graphics::BackBufferFormat;
    swapChainDesc.BufferDesc.ScanlineOrdering           = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling                    = DXGI_MODE_SCALING_UNSPECIFIED;

    // Sample description
    swapChainDesc.SampleDesc.Count                      = 1; // No MSAA for now
    swapChainDesc.SampleDesc.Quality                    = 0;

    swapChainDesc.BufferUsage                           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount                           = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.OutputWindow                          = Window::Handle;
    swapChainDesc.Windowed                              = true;
    swapChainDesc.SwapEffect                            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags                                 = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(
        DX12::Factory->CreateSwapChain(
            DX12::CommandQueue.Get(),
            &swapChainDesc,
            SwapChain::Chain.GetAddressOf()));
}

//
// Descriptor heaps
//
void CreateDescriptorHeaps() {
    //
    // RTV (render target view) heap
    //
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

    rtvHeapDesc.NumDescriptors  = SWAP_CHAIN_BUFFER_COUNT;
    rtvHeapDesc.Type            = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags           = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask        = 0;

    ThrowIfFailed(
        DX12::Device->CreateDescriptorHeap(
            &rtvHeapDesc,
            IID_PPV_ARGS(&RenderTarget::RtvHeap)));

    //
    // DSV (depth-stencil view) heap
    //
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};

    dsvHeapDesc.NumDescriptors  = DEPTH_STENCIL_BUFFER_COUNT;
    dsvHeapDesc.Type            = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags           = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask        = 0;

    ThrowIfFailed(
        DX12::Device->CreateDescriptorHeap(
            &dsvHeapDesc,
            IID_PPV_ARGS(&RenderTarget::DsvHeap)));
}

//
// GPU synchronization
//
void FlushQueue() {
    ++(Frame::CurrentFence);

    ThrowIfFailed(
        DX12::CommandQueue->Signal(
            DX12::Fence.Get(),
            Frame::CurrentFence));

    bool gpu_is_lagging_behind = DX12::Fence->GetCompletedValue() < Frame::CurrentFence;
    if (!gpu_is_lagging_behind) {
        return;
    }

    HANDLE event_handle = CreateEventEx(
        nullptr,
        nullptr,
        false,
        EVENT_ALL_ACCESS);

    // Wake until fence reaches current value or higher.
    ThrowIfFailed(
        DX12::Fence->SetEventOnCompletion(
            Frame::CurrentFence,
            event_handle));

    // Put CPU to sleep while GPU catches up.
    WaitForSingleObject(event_handle, INFINITE);
    CloseHandle(event_handle);
}

//
// Resize + RTV/DSV creation
//
void OnResize() {
    assert(DX12::Device);
    assert(DX12::CommandAllocator);
    assert(SwapChain::Chain);

    FlushQueue();

    //
    // Reset command list
    //
    ThrowIfFailed(
        DX12::CommandList->Reset(
            DX12::CommandAllocator.Get(),
            nullptr));

    //
    // Release old buffers
    //
    for (UINT i : std::views::iota(0, SWAP_CHAIN_BUFFER_COUNT))
        SwapChain::Buffers[i].Reset();
    RenderTarget::DepthStencilBuffer.Reset();

    //
    // Resize swap chain buffers
    //
    ThrowIfFailed(
        SwapChain::Chain->ResizeBuffers(
            SWAP_CHAIN_BUFFER_COUNT,
            Window::Width,
            Window::Height,
            Graphics::BackBufferFormat,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    Frame::CurrentBackBuffer = 0;

    //
    // Create RTVs
    //
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
        RenderTarget::RtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i : std::views::iota(0, SWAP_CHAIN_BUFFER_COUNT)) {
        ThrowIfFailed(
            SwapChain::Chain->GetBuffer(
                i,
                IID_PPV_ARGS(&SwapChain::Buffers[i])));

        DX12::Device->CreateRenderTargetView(
            SwapChain::Buffers[i].Get(),
            nullptr,
            rtv_handle);

        rtv_handle.ptr += Descriptor::RtvSize;
    }

    //
    // Create DSV buffer
    //
    D3D12_RESOURCE_DESC depthDescription   = {};
    depthDescription.Dimension             = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDescription.Alignment             = 0;
    depthDescription.Width                 = Window::Width;
    depthDescription.Height                = Window::Height;
    depthDescription.DepthOrArraySize      = 1;
    depthDescription.MipLevels             = 1;
    depthDescription.Format                = Graphics::DepthStencilFormat;
    depthDescription.SampleDesc.Count      = 1;
    depthDescription.SampleDesc.Quality    = 0;
    depthDescription.Layout                = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDescription.Flags                 = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue    = {};
    clearValue.Format               = Graphics::DepthStencilFormat;
    clearValue.DepthStencil.Depth   = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProperties    = {};
    heapProperties.Type                     = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty          = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference     = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask         = 1;
    heapProperties.VisibleNodeMask          = 1;

    ThrowIfFailed(
        DX12::Device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthDescription,
            D3D12_RESOURCE_STATE_COMMON,
            &clearValue,
            IID_PPV_ARGS(&RenderTarget::DepthStencilBuffer)));

    //
    // Create DSV
    //
    DX12::Device->CreateDepthStencilView(
        RenderTarget::DepthStencilBuffer.Get(),
        nullptr,
        RenderTarget::DsvHeap->GetCPUDescriptorHandleForHeapStart());

    //
    // Transition depth buffer
    //
    D3D12_RESOURCE_BARRIER depthBarrier =
        TransitionBarrier(
            RenderTarget::DepthStencilBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_DEPTH_WRITE);

    DX12::CommandList->ResourceBarrier(1, &depthBarrier);
    ThrowIfFailed(
        DX12::CommandList->Close());
    ID3D12CommandList* cmdsLists[] = { DX12::CommandList.Get() };
    DX12::CommandQueue->ExecuteCommandLists(1, cmdsLists);
    FlushQueue();

    //
    // Update viewport
    //
    Graphics::Viewport.TopLeftX  = 0;
    Graphics::Viewport.TopLeftY  = 0;
    Graphics::Viewport.Width     = static_cast<float>(Window::Width);
    Graphics::Viewport.Height    = static_cast<float>(Window::Height);
    Graphics::Viewport.MinDepth  = 0.0f;
    Graphics::Viewport.MaxDepth  = 1.0f;

    //
    // Update scissor rectangle
    //
    Graphics::ScissorRect.left   = 0;
    Graphics::ScissorRect.top    = 0;
    Graphics::ScissorRect.right  = Window::Width;
    Graphics::ScissorRect.bottom = Window::Height;
}

//
// Helper functions
//
ID3D12Resource* CurrentBackBuffer() {
    return SwapChain::Buffers[Frame::CurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() {
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        RenderTarget::RtvHeap->GetCPUDescriptorHandleForHeapStart();

    handle.ptr += static_cast<SIZE_T>(Frame::CurrentBackBuffer) * Descriptor::RtvSize;

    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() {
    return RenderTarget::DsvHeap->GetCPUDescriptorHandleForHeapStart();
}

//
// Draw
//
void Draw() {
    //
    // Reset command allocator and command list
    //
    ThrowIfFailed(
        DX12::CommandAllocator->Reset());

    ThrowIfFailed(
        DX12::CommandList->Reset(
            DX12::CommandAllocator.Get(),
            nullptr));

    //
    // Set viewport + scissor rectangle
    //
    DX12::CommandList->RSSetViewports(1, &Graphics::Viewport);
    DX12::CommandList->RSSetScissorRects(1, &Graphics::ScissorRect);

    //
    // Transition: PRESENT -> RENDER_TARGET
    //
    D3D12_RESOURCE_BARRIER toRenderTarget =
        TransitionBarrier(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

    DX12::CommandList->ResourceBarrier(1, &toRenderTarget);

    //
    // Bind render targets
    //
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = DepthStencilView();
    DX12::CommandList->OMSetRenderTargets(
        1,
        &rtv,
        true,
        &dsv);

    //
    // Clear RTV
    //
    float clearColor[] = {
        0.1f,
        0.2f,
        0.4f,
        1.0f
    };
    DX12::CommandList->ClearRenderTargetView(
        rtv,
        clearColor,
        0,
        nullptr);

    //
    // Clear DSV
    //
    DX12::CommandList->ClearDepthStencilView(
        dsv,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr);

    //
    // Transition: RENDER_TARGET -> PRESENT
    //
    D3D12_RESOURCE_BARRIER toPresent =
        TransitionBarrier(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    DX12::CommandList->ResourceBarrier(1, &toPresent);

    //
    // Send commands to GPU
    //
    ThrowIfFailed(DX12::CommandList->Close());
    ID3D12CommandList* commandLists[] = { DX12::CommandList.Get() };
    DX12::CommandQueue->ExecuteCommandLists(1, commandLists);

    //
    // Present backbuffer
    //
    ThrowIfFailed(
        SwapChain::Chain->Present(1, 0));
    Frame::CurrentBackBuffer = (Frame::CurrentBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

    FlushQueue();
}

D3D12_RESOURCE_BARRIER TransitionBarrier(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_TRANSITION_BARRIER transition = {};
    transition.pResource = resource;
    transition.StateBefore = before;
    transition.StateAfter = after;
    transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition = transition;

    return barrier;
}
