#pragma once

#include <windows.h>

namespace Acrylic::Window
{
void Init(HINSTANCE hInst,
          int nShowCmd);

auto GetHWnd() -> HWND;
auto GetWidth() -> int;
auto GetHeight() -> int;
} // namespace Acrylic::Window