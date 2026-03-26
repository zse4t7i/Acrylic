#pragma once

#include <windows.h>

namespace Acrylic::Window
{
void Init(HINSTANCE hInst,
          int nShowCmd);

auto GetHWnd() -> HWND;
auto GetWidth() -> int;
auto GetHeight() -> int;
auto IsResized() -> bool;
auto IsMinimized() -> bool;

void SetWidth(int width);
void SetHeight(int height);
void SetResized(bool resized);
void SetMinimized(bool minimized);
} // namespace Acrylic::Window