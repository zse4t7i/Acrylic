#include "Engine.hpp"
#include "Asset.hpp"
#include "Scene.hpp"
#include "UI.hpp"
#include "Log.hpp"
#include "Input.hpp"
#include "Timer.hpp"
#include "Window.hpp"
#include "D3D12.hpp"

namespace Acrylic::Engine
{
//==============================================================================
// External Function
//==============================================================================
void Init(HINSTANCE hInst)
{
    Acrylic::Engine::Log::Init("Log/Acrylic.log");
    Acrylic::Engine::Timer::Init();
    Acrylic::Engine::Input::Init();
    Acrylic::Engine::Window::Init(hInst);
    Acrylic::Engine::D3D12::Init();
    Acrylic::Engine::Asset::Load("");
    Acrylic::Engine::Scene::Init();
    Acrylic::Engine::UI::Init();
    LOG_INFO("Acrylic is ready!");
}

void Update()
{
    Acrylic::Engine::Timer::Update();
    Acrylic::Engine::Input::Update();
    Acrylic::Engine::Scene::Update();
    Acrylic::Engine::UI::Update();
}

void Render()
{
    Acrylic::Engine::Scene::Render();
    Acrylic::Engine::UI::Render();
}

void Present()
{
#ifdef DEBUG
    Acrylic::Engine::D3D12::PresentTear();
#else
    Acrylic::Engine::D3D12::PresentSync();
#endif
}

void Exit()
{
    Acrylic::Engine::UI::Exit();
    Acrylic::Engine::D3D12::Exit();
}

void ShowWindow(int nShowCmd)
{
    ShowWindow(Acrylic::Engine::Window::GetHWnd(), nShowCmd);
}

void WaitForNextFrame()
{
    Acrylic::Engine::D3D12::WaitForFrameBufferAvailable();
    Acrylic::Engine::D3D12::WaitForFrameResourceAvailable();
}
} // namespace Acrylic::Engine
