#include "framework.h"
#include "resource.h"

#include <cstdlib>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <stdexcept>
#include <windows.h>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

HINSTANCE gInstance = nullptr;
HWND gWindowHandle = nullptr;

constexpr WCHAR title[] = L"Asset Loader";
constexpr WCHAR windowClass[] = L"MainWindow";

LRESULT CALLBACK WindowProcess(HWND, UINT, WPARAM, LPARAM);
BOOL RegisterWndClass();
BOOL CreateWindowHandle(int commandShow);

int APIENTRY wWinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE previousInstance,
    _In_        LPWSTR commandLine,
    _In_        int commandShow)
{
    // Register global variables
    gInstance = hInstance;

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

BOOL RegisterWndClass() {
    WNDCLASSEXW wndClass = {};

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_VREDRAW | CS_HREDRAW;
	wndClass.lpfnWndProc = WindowProcess;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = gInstance;
	wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = windowClass;
	wndClass.hIconSm = NULL;

    if (RegisterClassExW(&wndClass) == 0)
        return false;

    return true;
}

BOOL CreateWindowHandle(const int commandShow) {
    HWND windowHandle = CreateWindowW(
        windowClass, title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT,
        0, nullptr, nullptr, gInstance, nullptr);

    if (!windowHandle)
        return false;

    gWindowHandle = windowHandle;
    ShowWindow(gWindowHandle, commandShow);
    UpdateWindow(gWindowHandle);

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
        PostQuitMessage(0);

	return DefWindowProc(windowHandle, message, wParam, lParam);
}
