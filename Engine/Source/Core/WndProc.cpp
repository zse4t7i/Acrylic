#include "Window.hpp"

// Forward declare message handler from imgui_impl_win32.cpp
extern auto ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                           LPARAM lParam) -> LRESULT;

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================

//==============================================================================
// Internal Function
//==============================================================================

} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Window
{
auto CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    -> LRESULT
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }

    switch (uMsg)
    {
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }

    case WM_SIZE: {
        Acrylic::Window::SetMinimized(wParam == SIZE_MINIMIZED);
        Acrylic::Window::SetWidth(LOWORD(lParam));
        Acrylic::Window::SetHeight(HIWORD(lParam));

        if (wParam != SIZE_MINIMIZED)
        {
            Acrylic::D3D12::Resize();
            // LOG_DEBUG("Resized.");
        }
        return 0;
    }

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
} // namespace Acrylic::Window
#pragma endregion