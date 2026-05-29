#include "Scene.hpp"
#include "Script.hpp"
#include "UI.hpp"

// Used to enable the "Agility SDK" components
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion =
        D3D12_AGILITY_SDK_VERSION;
    __declspec(dllexport) extern const char *D3D12SDKPath = "D3D12/";
}

auto WINAPI wWinMain(HINSTANCE hInst, HINSTANCE /*hPrevInst*/,
                     PWSTR /*pCmdLine*/, int nShowCmd) -> int
{
    { // Init
        Acrylic::Log::Init("Log/Acrylic.log");
        Acrylic::Window::Init(hInst);
        Acrylic::D3D12::Init();
        Acrylic::Scene::Init();
        Acrylic::UI::Init();
        LOG_INFO("Acrylic is ready!");
        Project::Script::Init();
    }

    ShowWindow(Acrylic::Window::GetHWnd(), nShowCmd);

    // Message loop.
    MSG msg{};
    while (true)
    {
        Acrylic::D3D12::WaitForBufferAvailable();
        Acrylic::D3D12::WaitForFrameAvailable();

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (msg.message == WM_QUIT)
        {
            break;
        }

        { // Update
            Acrylic::Scene::Update();
            Acrylic::UI::Update();
        }
        { // Render
            Acrylic::Scene::Render();
            Acrylic::UI::Render();
        }

        { // Present
#ifdef DEBUG
            Acrylic::D3D12::PresentTear();
#else
            Acrylic::D3D12::PresentSync();
#endif
        }
    }

    return static_cast<int>(msg.wParam);
}