#include "Engine.hpp"
#include "Asset.hpp"
#include "Scene.hpp"
#include "UI.hpp"

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

void Exit()
{
    Acrylic::Engine::UI::Exit();
    Acrylic::Engine::D3D12::Exit();
}

} // namespace Acrylic::Engine
