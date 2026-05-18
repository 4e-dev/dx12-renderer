#include <windows.h>
#include <wrl.h>

#include <cassert>
#include <cstdlib>
#include <stdexcept>

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

constexpr UINT SwapChainBufferCount = 2;

//
// Globals
//
HINSTANCE gInstance = nullptr;
HWND gWindowHandle = nullptr;

int gWidth = 1920;
int gHeight = 1080;

bool gPaused = false; // stops rendering if true
bool gMinimized = false; // only referenced in WM_SIZE

UINT gCurrentBackBuffer = 0;

UINT gRtvDescriptorSize = 0;
UINT gDsvDescriptorSize = 0;
UINT gCbvSrvUavDescriptorSize = 0;

UINT64 gCurrentFence = 0;

DXGI_FORMAT gBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
DXGI_FORMAT gDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

D3D12_VIEWPORT gViewport = {};
D3D12_RECT gScissorRect = {};

//
// DX12 objects
//
ComPtr<IDXGIFactory4>               gDxgiFactory;
ComPtr<ID3D12Device>                gDevice;
ComPtr<ID3D12Fence>                 gFence;

ComPtr<ID3D12CommandQueue>          gCommandQueue;
ComPtr<ID3D12CommandAllocator>      gCommandAllocator;
ComPtr<ID3D12GraphicsCommandList>   gCommandList;

ComPtr<IDXGISwapChain>              gSwapChain;

ComPtr<ID3D12DescriptorHeap>        gRtvHeap;
ComPtr<ID3D12DescriptorHeap>        gDsvHeap;

ComPtr<ID3D12Resource>              gSwapChainBuffers[SwapChainBufferCount];
ComPtr<ID3D12Resource>              gDepthStencilBuffer;

//
// Helpers
//
void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error("HRESULT failed.");
    }
}

D3D12_RESOURCE_BARRIER TransitionBarrier(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier = {};

    barrier.Type    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags   = D3D12_RESOURCE_BARRIER_FLAG_NONE;

    barrier.Transition.pResource    = resource;
    barrier.Transition.StateBefore  = before;
    barrier.Transition.StateAfter   = after;
    barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    return barrier;
}

//
// Forward declarations
//
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void CreateDevice();
void CreateFence();
void GetDescriptorSizes();
void CheckMsaaSupport();

void CreateCommandObjects();
void CreateSwapChain();
void CreateDescriptorHeaps();

void FlushCommandQueue();
void OnResize();

void Draw();

//
// Window procedure
//
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_KEYDOWN: // Key being pressed
            {
                if (wParam == VK_ESCAPE)
                {
                    PostQuitMessage(0);
                }
                return 0;
            }
        case WM_DESTROY: // Destroy window
            {
                PostQuitMessage(0);
                return 0;
            }
        case WM_CLOSE: // Pressing the close ('X') button
            {
                DestroyWindow(window);
                return 0;
            }
        case WM_SIZE: // Window is resized
            {
                gWidth = LOWORD(lParam);
                gHeight = HIWORD(lParam);

                if (gDevice)
                {
                    if (wParam == SIZE_MINIMIZED)
                    {
                        // No rendering when minimized!
                        gPaused = true;
                        gMinimized = true;
                    }
                    else if (wParam == SIZE_MAXIMIZED)
                    {
                        gPaused = false;
                        gMinimized = false;
                        OnResize();
                    }
                    else if (wParam == SIZE_RESTORED)
                    {
                        // Ordinary window state (non-maximized, non-minimized)
                        if (gMinimized)
                        {
                            gPaused = false;
                            gMinimized = false;
                            OnResize();
                        }
                    }
                }
            }
    }

    return DefWindowProc(window, message, wParam, lParam);
}

//
// Entry point
//
int APIENTRY wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPWSTR,
    int commandShow)
{
    gInstance = hInstance;

    //
    // Register window class
    //
    WNDCLASSEXW wc = {};

    wc.cbSize           = sizeof(WNDCLASSEXW);
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WindowProc;
    wc.hInstance        = gInstance;
    wc.hIcon            = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName    = L"D3D12WindowClass";

    if (!RegisterClassExW(&wc))
    {
        return EXIT_FAILURE;
    }

    //
    // Create window
    //
    gWindowHandle = CreateWindowW(
        L"D3D12WindowClass",
        L"D3D12 Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        gWidth,
        gHeight,
        nullptr,
        nullptr,
        gInstance,
        nullptr);

    if (!gWindowHandle)
    {
        return EXIT_FAILURE;
    }

    ShowWindow(gWindowHandle, commandShow);
    UpdateWindow(gWindowHandle);

    //
    // Initialize D3D12
    //
    try
    {
        CreateDevice();
        CreateFence();
        GetDescriptorSizes();
        CheckMsaaSupport();
        CreateCommandObjects();
        CreateSwapChain();
        CreateDescriptorHeaps();
        OnResize();
    }
    catch (...)
    {
        MessageBoxA(
            nullptr,
            "Failed to initialize D3D12.",
            "Error",
            MB_OK);

        return EXIT_FAILURE;
    }

    //
    // Main loop
    //
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
            if (!gPaused)
            {
                Draw();
            }
        }
    }

    FlushCommandQueue();

    return static_cast<int>(message.wParam);
}

//
// Device
//
void CreateDevice()
{
#if defined(DEBUG) || defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;

        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

    ThrowIfFailed(
        CreateDXGIFactory1(IID_PPV_ARGS(&gDxgiFactory)));

    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&gDevice));

    if (FAILED(hardwareResult))
    {
        // Fall back to WARP (software rasterizer)
        ComPtr<IDXGIAdapter> warpAdapter;

        ThrowIfFailed(
            gDxgiFactory->EnumWarpAdapter(
                IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(
            D3D12CreateDevice(
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&gDevice)));
    }
}

void CreateFence()
{
    ThrowIfFailed(
        gDevice->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&gFence)));
}

void GetDescriptorSizes()
{
    gRtvDescriptorSize =
        gDevice->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    gDsvDescriptorSize =
        gDevice->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    gCbvSrvUavDescriptorSize =
        gDevice->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CheckMsaaSupport()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQuality = {};

    msaaQuality.Format = gBackBufferFormat;
    msaaQuality.SampleCount = 4;
    msaaQuality.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaQuality.NumQualityLevels = 0;

    ThrowIfFailed(
        gDevice->CheckFeatureSupport(
            D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &msaaQuality,
            sizeof(msaaQuality)));
}

//
// Command objects
//
void CreateCommandObjects()
{
    //
    // Command queue
    //
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(
        gDevice->CreateCommandQueue(
            &queueDesc,
            IID_PPV_ARGS(&gCommandQueue)));

    //
    // Command allocator
    //
    ThrowIfFailed(
        gDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&gCommandAllocator)));

    //
    // Command list
    //
    ThrowIfFailed(
        gDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            gCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&gCommandList)));

    //
    // Close immediately
    //
    ThrowIfFailed(
        gCommandList->Close());
}

//
// Swap chain
//
void CreateSwapChain()
{
    gSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    swapChainDesc.BufferDesc.Width = gWidth;
    swapChainDesc.BufferDesc.Height = gHeight;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = gBackBufferFormat;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    //
    // No MSAA for now
    //
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SwapChainBufferCount;
    swapChainDesc.OutputWindow = gWindowHandle;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(
        gDxgiFactory->CreateSwapChain(
            gCommandQueue.Get(),
            &swapChainDesc,
            gSwapChain.GetAddressOf()));
}

//
// Descriptor heaps
//
void CreateDescriptorHeaps()
{
    //
    // RTV heap
    //
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;

    ThrowIfFailed(
        gDevice->CreateDescriptorHeap(
            &rtvHeapDesc,
            IID_PPV_ARGS(&gRtvHeap)));

    //
    // DSV heap
    //
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};

    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;

    ThrowIfFailed(
        gDevice->CreateDescriptorHeap(
            &dsvHeapDesc,
            IID_PPV_ARGS(&gDsvHeap)));
}

//
// GPU synchronization
//
void FlushCommandQueue()
{
    ++gCurrentFence;

    ThrowIfFailed(
        gCommandQueue->Signal(
            gFence.Get(),
            gCurrentFence));

    if (gFence->GetCompletedValue() < gCurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(
            nullptr,
            nullptr,
            false,
            EVENT_ALL_ACCESS);

        ThrowIfFailed(
            gFence->SetEventOnCompletion(
                gCurrentFence,
                eventHandle));

        WaitForSingleObject(eventHandle, INFINITE);

        CloseHandle(eventHandle);
    }
}

//
// Resize + RTV/DSV creation
//
void OnResize()
{
    assert(gDevice);
    assert(gSwapChain);
    assert(gCommandAllocator);

    FlushCommandQueue();

    //
    // Reset command list
    //
    ThrowIfFailed(
        gCommandList->Reset(
            gCommandAllocator.Get(),
            nullptr));

    //
    // Release old buffers
    //
    for (UINT i = 0; i < SwapChainBufferCount; ++i)
        gSwapChainBuffers[i].Reset();

    gDepthStencilBuffer.Reset();

    //
    // Resize swap chain buffers
    //
    ThrowIfFailed(
        gSwapChain->ResizeBuffers(
            SwapChainBufferCount,
            gWidth,
            gHeight,
            gBackBufferFormat,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    gCurrentBackBuffer = 0;

    //
    // Create RTVs
    //
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        gRtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < SwapChainBufferCount; ++i)
    {
        ThrowIfFailed(
            gSwapChain->GetBuffer(
                i,
                IID_PPV_ARGS(&gSwapChainBuffers[i])));

        gDevice->CreateRenderTargetView(
            gSwapChainBuffers[i].Get(),
            nullptr,
            rtvHandle);

        rtvHandle.ptr += gRtvDescriptorSize;
    }

    //
    // Create depth/stencil buffer
    //
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = gWidth;
    depthDesc.Height = gHeight;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = gDepthStencilFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = gDepthStencilFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    ThrowIfFailed(
        gDevice->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_COMMON,
            &clearValue,
            IID_PPV_ARGS(&gDepthStencilBuffer)));

    //
    // Create DSV
    //
    gDevice->CreateDepthStencilView(
        gDepthStencilBuffer.Get(),
        nullptr,
        gDsvHeap->GetCPUDescriptorHandleForHeapStart());

    //
    // Transition depth buffer
    //
    D3D12_RESOURCE_BARRIER depthBarrier =
        TransitionBarrier(
            gDepthStencilBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_DEPTH_WRITE);

    gCommandList->ResourceBarrier(1, &depthBarrier);

    ThrowIfFailed(gCommandList->Close());

    ID3D12CommandList* cmdsLists[] =
    {
        gCommandList.Get()
    };

    gCommandQueue->ExecuteCommandLists(1, cmdsLists);

    FlushCommandQueue();

    //
    // Viewport
    //
    gViewport.TopLeftX = 0;
    gViewport.TopLeftY = 0;
    gViewport.Width = static_cast<float>(gWidth);
    gViewport.Height = static_cast<float>(gHeight);
    gViewport.MinDepth = 0.0f;
    gViewport.MaxDepth = 1.0f;

    //
    // Scissor rect
    //
    gScissorRect.left = 0;
    gScissorRect.top = 0;
    gScissorRect.right = gWidth;
    gScissorRect.bottom = gHeight;
}

//
// Helpers
//
ID3D12Resource* CurrentBackBuffer()
{
    return gSwapChainBuffers[gCurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        gRtvHeap->GetCPUDescriptorHandleForHeapStart();

    handle.ptr += static_cast<SIZE_T>(gCurrentBackBuffer) * gRtvDescriptorSize;

    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()
{
    return gDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

//
// Draw
//
void Draw()
{
    ThrowIfFailed(
        gCommandAllocator->Reset());

    ThrowIfFailed(
        gCommandList->Reset(
            gCommandAllocator.Get(),
            nullptr));

    //
    // Set viewport + scissor
    //
    gCommandList->RSSetViewports(1, &gViewport);
    gCommandList->RSSetScissorRects(1, &gScissorRect);

    //
    // Transition: PRESENT -> RENDER_TARGET
    //
    D3D12_RESOURCE_BARRIER toRenderTarget =
        TransitionBarrier(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

    gCommandList->ResourceBarrier(1, &toRenderTarget);

    //
    // Get RTV + DSV handles
    //
    D3D12_CPU_DESCRIPTOR_HANDLE rtv =
        CurrentBackBufferView();

    D3D12_CPU_DESCRIPTOR_HANDLE dsv =
        DepthStencilView();

    //
    // Bind render targets
    //
    gCommandList->OMSetRenderTargets(
        1,
        &rtv,
        true,
        &dsv);

    //
    // Clear RTV
    //
    float clearColor[] =
    {
        0.1f,
        0.2f,
        0.4f,
        1.0f
    };

    gCommandList->ClearRenderTargetView(
        rtv,
        clearColor,
        0,
        nullptr);

    //
    // Clear DSV
    //
    gCommandList->ClearDepthStencilView(
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

    gCommandList->ResourceBarrier(1, &toPresent);

    ThrowIfFailed(
        gCommandList->Close());

    ID3D12CommandList* commandLists[] =
    {
        gCommandList.Get()
    };

    gCommandQueue->ExecuteCommandLists(
        1,
        commandLists);

    ThrowIfFailed(
        gSwapChain->Present(1, 0));

    //
    // Advance back buffer
    //
    gCurrentBackBuffer =
        (gCurrentBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}
