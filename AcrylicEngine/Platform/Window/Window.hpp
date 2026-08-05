#pragma once

// Forward declare message handler from imgui_impl_win32.cpp
extern auto ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                           UINT msg,
                                           WPARAM wParam,
                                           LPARAM lParam) -> LRESULT;

namespace Acrylic::Engine::Window
{
void Init(HINSTANCE hInst);
auto CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    -> LRESULT;

auto GetHWnd() -> HWND;
auto GetWidth() -> int;
auto GetHeight() -> int;
auto GetMinimized() -> bool;

void SetWidth(int width);
void SetHeight(int height);
void SetMinimized(bool minimized);
} // namespace Acrylic::Engine::Window