#include "D3D12.hpp"
#include "Log.hpp"

// Used to enable the "Agility SDK" components
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char* D3D12SDKPath = R"(.\Asset\D3D12\)";
}

auto WINAPI wWinMain(HINSTANCE hInst,
                     HINSTANCE /*hPrevInst*/,
                     PWSTR /*pCmdLine*/,
                     int nShowCmd) -> int
{
    Acrylic::Log::Init(LR"(Log\Acrylic.log)");
    LOG_INFO("Acrylic is ready!");

    Acrylic::D3D12::Init(hInst, nShowCmd);

    // Message loop.
    MSG _msg{};
    while (true)
    {
        if (PeekMessageW(&_msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (_msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&_msg);
            DispatchMessageW(&_msg);
        }

        Acrylic::D3D12::Update();
        Acrylic::D3D12::Render();
    }

    Acrylic::D3D12::Destroy();

    return static_cast<int>(_msg.wParam);
}
