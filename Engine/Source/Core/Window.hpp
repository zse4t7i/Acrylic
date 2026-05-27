#pragma once

namespace Acrylic::Window
{
void Init(HINSTANCE hInst);

auto GetHWnd() -> HWND;
auto GetWidth() -> int;
auto GetHeight() -> int;
auto GetMinimized() -> bool;

void SetWidth(int width);
void SetHeight(int height);
void SetMinimized(bool minimized);
} // namespace Acrylic::Window