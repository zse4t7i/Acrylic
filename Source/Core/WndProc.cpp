#include "Window.hpp"

#include <windows.h>

namespace
{
//==============================================================================
// Variable
//==============================================================================

//==============================================================================
// Function
//==============================================================================

} // namespace

namespace Acrylic::Window
{
auto CALLBACK WndProc(HWND hWnd,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam) -> LRESULT
{
    switch (uMsg)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }

    case WM_SIZE:
    {
        Acrylic::Window::SetResized(true);
        Acrylic::Window::SetMinimized(wParam == SIZE_MINIMIZED);
        Acrylic::Window::SetWidth(LOWORD(lParam));
        Acrylic::Window::SetHeight(HIWORD(lParam));
        return 0;
    }

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
} // namespace Acrylic::Window
