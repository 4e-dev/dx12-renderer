#include "framework.h"
#include "resource.h"

#include <cstdlib>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <stdexcept>
#include <windows.h>
#include <wrl.h>

HINSTANCE gInstance;
constexpr WCHAR title[] = L"Asset Loader";
constexpr WCHAR windowClass[] = L"MainWindow";

LRESULT CALLBACK WindowProcess(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE previousInstance,
    _In_        LPWSTR commandLine,
    _In_        int commandShow)
{
    // Register a new window
    WNDCLASSEXW wndClass = {};

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_VREDRAW | CS_HREDRAW; // redraw upon resizes
	wndClass.lpfnWndProc = WindowProcess;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = windowClass;
	wndClass.hIconSm = NULL;

    if (RegisterClassExW(&wndClass) == 0)
        return EXIT_FAILURE;

    // Create the main window
    gInstance = hInstance;

    HWND windowHandle = CreateWindowW(
        windowClass, title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT,
        0, nullptr, nullptr, hInstance, nullptr);

    if (!windowHandle)
        return EXIT_FAILURE;

    ShowWindow(windowHandle, commandShow);
    UpdateWindow(windowHandle);

    // Main message loop:
    MSG message;
    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return (int) message.wParam;
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
