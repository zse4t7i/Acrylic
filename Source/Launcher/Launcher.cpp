#include "Engine.hpp"

#include <windows.h>

// Used to enable the "Agility SDK" components
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion =
        D3D12_AGILITY_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = "D3D12/";
}

auto WINAPI wWinMain(HINSTANCE hInst,
                     HINSTANCE /*hPrevInst*/,
                     PWSTR /*pCmdLine*/,
                     int nShowCmd) -> int
{
    Acrylic::Engine::Init(hInst);

    Acrylic::Engine::ShowWindow(nShowCmd);

    // Message loop.
    MSG msg{};
    while (true)
    {
        Acrylic::Engine::WaitForNextFrame();

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

        Acrylic::Engine::Update();
        Acrylic::Engine::Render();
        Acrylic::Engine::Present();
    }

    Acrylic::Engine::Exit();

    return static_cast<int>(msg.wParam);
}