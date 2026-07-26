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
    Acrylic::Log::Init("Log/Acrylic.log");
    Acrylic::Timer::Init();
    Acrylic::Input::Init();
    Acrylic::Window::Init(hInst);
    Acrylic::D3D12::Init();
    Acrylic::Asset::Load("");
    Acrylic::Scene::Init();
    Acrylic::UI::Init();
    LOG_INFO("Acrylic is ready!");
}

void Update()
{
    Acrylic::Timer::Update();
    Acrylic::Input::Update();
    Acrylic::Scene::Update();
    Acrylic::UI::Update();
}

void Render()
{
    Acrylic::Scene::Render();
    Acrylic::UI::Render();
}

void Exit()
{
    Acrylic::UI::Exit();
    Acrylic::D3D12::Exit();
}

} // namespace Acrylic::Engine
