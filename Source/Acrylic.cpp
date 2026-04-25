#include "D3D12.hpp"
#include "Frame.hpp"
#include "Log.hpp"
#include "Scene.hpp"
#include "Window.hpp"

#include <Windows.h>

// Used to enable the "Agility SDK" components
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion =
        D3D12_AGILITY_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = R"(.\D3D12\)";
}

auto WINAPI wWinMain(HINSTANCE hInst,
                     HINSTANCE /*hPrevInst*/,
                     PWSTR /*pCmdLine*/,
                     int nShowCmd) -> int
{
    Acrylic::Log::Init(LR"(Log\Acrylic.log)");
    Acrylic::Window::Init(hInst, nShowCmd);
    Acrylic::D3D12::Init();
    Acrylic::Frame::Init();
    Acrylic::Scene::Init();
    LOG_INFO("Acrylic is ready!");

    ShowWindow(Acrylic::Window::GetHWnd(), nShowCmd);

    // Message loop.
    MSG msg{};
    while (true)
    {
        Acrylic::D3D12::WaitForSwapChain();
        Acrylic::Frame::MoveToNext();

        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (Acrylic::Window::IsResized() && !Acrylic::Window::IsMinimized())
        {
            Acrylic::Window::SetResized(false);
            Acrylic::D3D12::Resize();
        }

        Acrylic::Scene::Update();
        Acrylic::Scene::Render();
    }

    return static_cast<int>(msg.wParam);
}
