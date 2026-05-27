#include "Window.hpp"

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
HWND HWnd{};

int Width{1920};
int Height{1200};

bool Minimized{false};

//==============================================================================
// Internal Function
//==============================================================================

} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Window
{
auto CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    -> LRESULT;

void Init(HINSTANCE hInst)
{
    // This makes sure that in a multi-monitor setup with different resolutions,
    // get monitor info returns correct dimensions.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIconW(hInst, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"AcrylicMainWindowClass";
    wcex.hIconSm = LoadIconW(hInst, IDI_APPLICATION);
    RegisterClassExW(&wcex);

    RECT rc{0, 0, Width, Height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);

    // Create a window.
    HWnd = CreateWindowExW(0, L"AcrylicMainWindowClass", L"Acrylic",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           rc.right - rc.left, rc.bottom - rc.top, nullptr,
                           nullptr, hInst, nullptr);

    LOG_INFO("Acrylic::Window::Init() succeeded.");
}

//==============================================================================
// Accessors
//==============================================================================
auto GetHWnd() -> HWND
{
    return HWnd;
}
auto GetWidth() -> int
{
    return Width;
}
auto GetHeight() -> int
{
    return Height;
}
auto GetMinimized() -> bool
{
    return Minimized;
}

void SetWidth(int width)
{
    Width = width;
}
void SetHeight(int height)
{
    Height = height;
}
void SetMinimized(bool minimized)
{
    Minimized = minimized;
}

} // namespace Acrylic::Window
#pragma endregion