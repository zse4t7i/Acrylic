#include "Engine.hpp"
#include "Asset.hpp"
#include "D3D12.hpp"
#include "Input.hpp"
#include "Log.hpp"
#include "Scene.hpp"
#include "Timer.hpp"
#include "UI.hpp"
#include "Window.hpp"

using namespace Acrylic;

namespace Acrylic::Engine
{
//==============================================================================
// External Function
//==============================================================================
void Init(HINSTANCE hInst)
{
    Log::Init("Log/AcrylicEngine.log");
    Timer::Init();
    Input::Init();
    Window::Init(hInst);
    D3D12::Init();
    Asset::Load("");
    Scene::Init();
    UI::Init();
    LOG_INFO("AcrylicEngine is initialized!");
}

void Update()
{
    Timer::Update();
    Input::Update();
    Scene::Update();
    UI::Update();
}

void Render()
{
    Scene::Render();
    UI::Render();
}

void Present()
{
#ifdef DEBUG
    D3D12::PresentTear();
#else
    D3D12::PresentSync();
#endif
}

void Exit()
{
    UI::Exit();
    D3D12::Exit();
}

void ShowWindow(int nShowCmd)
{
    ::ShowWindow(Window::GetHWnd(), nShowCmd);
}

void WaitForNextFrame()
{
    D3D12::WaitForFrameBufferAvailable();
    D3D12::WaitForFrameResourceAvailable();
}
} // namespace Acrylic::Engine
