#include "UI.hpp"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
// Return result for assertion.
HRESULT HR{};
bool BR{};

constexpr int DESCIPTORCOUNT{64};
stack<int> HeapIndices{};

// External D3D12 Objects
ID3D12Device9 *Device;
ID3D12CommandQueue *CmdQueue;

// Internal D3D12 Objects
ComPtr<ID3D12DescriptorHeap> HeapSRV{};
ComPtr<ID3D12GraphicsCommandList6> CmdList{};
array<ComPtr<ID3D12CommandAllocator>, Acrylic::D3D12::FRAMECOUNT>
    CmdAlctrs{};
//==============================================================================
// Internal Function
//==============================================================================
void InitInternalD3D12Objects()
{
    { // Create HeapCSU
        // The first SRV is reserved for the font texture used by ImGui.
        D3D12_DESCRIPTOR_HEAP_DESC descHeapSRV{
            .Type{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
            .NumDescriptors{DESCIPTORCOUNT},
            .Flags{D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE}};

        HR = Device->CreateDescriptorHeap(&descHeapSRV,
                                          IID_PPV_ARGS(HeapSRV.GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create CSU descriptor heap.");
    }

    HR = Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    D3D12_COMMAND_LIST_FLAG_NONE,
                                    IID_PPV_ARGS(CmdList.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create command list.");

    for (int i = 0; i < Acrylic::D3D12::FRAMECOUNT; i++)
    {
        HR = Acrylic::D3D12::GetDevice()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CmdAlctrs[i].GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create command allocator.");
    }
}

void InitImGuiBackend()
{
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(Acrylic::Window::GetHWnd());

    for (int i = DESCIPTORCOUNT - 1; i >= 0; i--)
    {
        HeapIndices.push(i);
    }

    ImGui_ImplDX12_InitInfo initInfo{};
    initInfo.Device = Acrylic::D3D12::GetDevice();
    initInfo.CommandQueue = Acrylic::D3D12::GetCmdQueue();
    initInfo.NumFramesInFlight = Acrylic::D3D12::FRAMECOUNT;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap = HeapSRV.Get();
    initInfo.SrvDescriptorAllocFn =
        [](ImGui_ImplDX12_InitInfo *, D3D12_CPU_DESCRIPTOR_HANDLE *outHandleCPU,
           D3D12_GPU_DESCRIPTOR_HANDLE *outHandleGPU) -> void {
        assert(!HeapIndices.empty() &&
               "No more descriptors available in the heap.");

        int index = HeapIndices.top();
        HeapIndices.pop();
        outHandleCPU->ptr = HeapSRV->GetCPUDescriptorHandleForHeapStart().ptr +
                            (index * Acrylic::D3D12::GetStrideCSU());
        outHandleGPU->ptr = HeapSRV->GetGPUDescriptorHandleForHeapStart().ptr +
                            (index * Acrylic::D3D12::GetStrideCSU());
    };
    initInfo.SrvDescriptorFreeFn =
        [](ImGui_ImplDX12_InitInfo *, D3D12_CPU_DESCRIPTOR_HANDLE handleCPU,
           D3D12_GPU_DESCRIPTOR_HANDLE handleGPU) -> void {
        int indexCPU = static_cast<int>(
            (handleCPU.ptr -
             HeapSRV->GetCPUDescriptorHandleForHeapStart().ptr) /
            Acrylic::D3D12::GetStrideCSU());
        int indexGPU = static_cast<int>(
            (handleGPU.ptr -
             HeapSRV->GetGPUDescriptorHandleForHeapStart().ptr) /
            Acrylic::D3D12::GetStrideCSU());
        assert(indexCPU == indexGPU &&
               "CPU and GPU descriptor index do not match.");
        HeapIndices.push(indexCPU);
    };
    ImGui_ImplDX12_Init(&initInfo);
}
} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::UI
{
//==============================================================================
// External Function
//==============================================================================
void Init()
{
    Device = Acrylic::D3D12::GetDevice();
    CmdQueue = Acrylic::D3D12::GetCmdQueue();

    { // Init ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;
        // io.Fonts->AddFontDefaultVector();
        io.Fonts->AddFontFromFileTTF("Font/MiSansNFP.ttf");
        io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\seguivar.ttf)");

        ImGuiStyle &style = ImGui::GetStyle();
        style.FontSizeBase = 20.0F;

        ImGui::StyleColorsLight();
    }

    InitInternalD3D12Objects();
    InitImGuiBackend();

    LOG_INFO("Acrylic::UI::Init() succeeded.");
}

void Update()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    { // Draw UI here!
        ImGui::ShowDemoWindow();
        ImGui::ShowMetricsWindow();
    }

    ImGui::Render();
}

void Render()
{
    auto *currentRT = Acrylic::D3D12::GetCurrentRT();
    auto currentRTV = Acrylic::D3D12::GetCurrentRTV();
    auto frameIndex = Acrylic::D3D12::GetFrameIndex();

    HR = CmdAlctrs[frameIndex]->Reset();
    assert(SUCCEEDED(HR) && "Failed to reset command allocator.");
    HR = CmdList->Reset(CmdAlctrs[frameIndex].Get(), nullptr);
    assert(SUCCEEDED(HR) && "Failed to reset command list.");

    auto p2r = CD3DX12_RESOURCE_BARRIER::Transition(
        currentRT, D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &p2r);

    CmdList->OMSetRenderTargets(1, &currentRTV, false, nullptr);

    vector<ID3D12DescriptorHeap *> heaps{HeapSRV.Get()};
    CmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CmdList.Get());

    auto r2p = CD3DX12_RESOURCE_BARRIER::Transition(
        currentRT, D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &r2p);

    HR = CmdList->Close();
    assert(SUCCEEDED(HR) && "Failed to close command list.");

    vector<ID3D12CommandList *> cmdLists{CmdList.Get()};
    CmdQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
}

// void Exit()
// {
//     Acrylic::D3D12::WaitForCmdExecuted();
//     ImGui_ImplDX12_Shutdown();
//     ImGui_ImplWin32_Shutdown();
//     ImGui::DestroyContext();
// }
} // namespace Acrylic::UI
#pragma endregion