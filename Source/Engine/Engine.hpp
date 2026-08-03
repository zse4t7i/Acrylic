#pragma once

#include <Windows.h>

namespace Acrylic::Engine
{
void Init(HINSTANCE hInst);
void Update();
void Render();
void Present();
void Exit();

void ShowWindow(int nShowCmd);
void WaitForNextFrame();
} // namespace Acrylic::Engine::Engine