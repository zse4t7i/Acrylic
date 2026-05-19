#include "D3D12.hpp"
#include "Log.hpp"
#include "Util.hpp"
#include "Window.hpp"

#include <D3D12MemAlloc.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include <dxgi1_6.h>
#include <windows.h>
#include <wrl/client.h>
#ifdef DEBUG
#include <dxgidebug.h>
#endif

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
HRESULT hr{};
bool br{};

std::string GPUName{};

HANDLE EventSwapChain{};

ComPtr<ID3D12Device9> Device{};
ComPtr<D3D12MA::Allocator> MemAllocator{};
ComPtr<ID3D12GraphicsCommandList6> CmdList{};
ComPtr<ID3D12CommandQueue> CmdQueue{};

ComPtr<IDXGISwapChain4> SwapChain{};
constexpr int BUFFERCOUNT{3};
std::array<ComPtr<ID3D12Resource>, BUFFERCOUNT> RTs{};
ComPtr<ID3D12DescriptorHeap> HeapRTV{};
std::uint32_t OffsetRTV{0};

//==============================================================================
// Internal Function
//==============================================================================
void InitGraphicsPipeline()
{
    ComPtr<ID3D12Debug5> debugLayer{};
    ComPtr<IDXGIFactory6> factory{};
    ComPtr<IDXGIAdapter4> adapter{};
    ComPtr<ID3D12InfoQueue> infoQueue{};

#pragma region Device
    {
        std::uint32_t dxgiFactoryFlags = 0;

#ifdef DEBUG
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to enable D3D12 debug layer.");
        debugLayer->EnableDebugLayer();
        debugLayer->SetEnableGPUBasedValidation(true);
        debugLayer->SetEnableAutoName(true);
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        hr = CreateDXGIFactory2(dxgiFactoryFlags,
                                IID_PPV_ARGS(factory.GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to create DXGI factory.");

        hr = factory->EnumAdapterByGpuPreference(
            0,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(adapter.GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to find a dGPU.");

        DXGI_ADAPTER_DESC1 descAdapter{};
        adapter->GetDesc1(&descAdapter);
        Acrylic::Util::UTF1628(descAdapter.Description, GPUName);

        hr = D3D12CreateDevice(adapter.Get(),
                               D3D_FEATURE_LEVEL_12_1,
                               IID_PPV_ARGS(Device.GetAddressOf()));
        assert(SUCCEEDED(hr)
               && "Failed to find a dGPU that supports D3D12 "
                  "Feature Level 12_1.");

#ifdef DEBUG
        hr = Device.As(&infoQueue);
        assert(SUCCEEDED(hr) && "Failed to query ID3D12InfoQueue.");
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

        std::vector<D3D12_MESSAGE_ID> denyIds{
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE};

        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs  = denyIds.size();
        filter.DenyList.pIDList = denyIds.data();

        infoQueue->AddStorageFilterEntries(&filter);
#endif
    }
#pragma endregion

#pragma region D3D12MA
    {
        D3D12MA::ALLOCATOR_DESC descD3D12MA{};
        descD3D12MA.Flags    = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS;
        descD3D12MA.pDevice  = Device.Get();
        descD3D12MA.pAdapter = adapter.Get();

        hr =
            D3D12MA::CreateAllocator(&descD3D12MA, MemAllocator.GetAddressOf());
        assert(SUCCEEDED(hr) && "Failed to create D3D12 Memory Allocator.");
    }
#pragma endregion

#pragma region CQ & CL
    {
        D3D12_COMMAND_QUEUE_DESC descCQ{};
        descCQ.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        descCQ.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;

        hr = Device->CreateCommandQueue(&descCQ,
                                        IID_PPV_ARGS(CmdQueue.GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to create command queue.");

        hr = Device->CreateCommandList1(0,
                                        D3D12_COMMAND_LIST_TYPE_DIRECT,
                                        D3D12_COMMAND_LIST_FLAG_NONE,
                                        IID_PPV_ARGS(CmdList.GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to create command list.");
    }
#pragma endregion

#pragma region SwapChain
    {
        DXGI_SWAP_CHAIN_DESC1 descSwapChain{};
        descSwapChain.BufferCount      = BUFFERCOUNT;
        descSwapChain.Width            = Acrylic::Window::GetWidth();
        descSwapChain.Height           = Acrylic::Window::GetHeight();
        descSwapChain.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        descSwapChain.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        descSwapChain.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        descSwapChain.SampleDesc.Count = 1;
        descSwapChain.Flags =
            DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
            | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        ComPtr<IDXGISwapChain1> swapChain{};
        hr = factory->CreateSwapChainForHwnd(CmdQueue.Get(),
                                             Acrylic::Window::GetHWnd(),
                                             &descSwapChain,
                                             nullptr,
                                             nullptr,
                                             swapChain.GetAddressOf());
        assert(SUCCEEDED(hr) && "Failed to create swap chain.");
        hr = swapChain.As(&SwapChain);
        assert(SUCCEEDED(hr) && "Failed to query IDXGISwapChain4.");

        hr = SwapChain->SetMaximumFrameLatency(2);
        assert(SUCCEEDED(hr) && "Failed to SetMaximumFrameLatency(2).");
        EventSwapChain = SwapChain->GetFrameLatencyWaitableObject();

        factory->MakeWindowAssociation(Acrylic::Window::GetHWnd(),
                                       DXGI_MWA_NO_ALT_ENTER);
    }
#pragma endregion

#pragma region RTV
    {
        D3D12_DESCRIPTOR_HEAP_DESC descHeapRTV{};
        descHeapRTV.NumDescriptors = BUFFERCOUNT;
        descHeapRTV.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        descHeapRTV.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        hr = Device->CreateDescriptorHeap(&descHeapRTV,
                                          IID_PPV_ARGS(HeapRTV.GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to create RTV descriptor heap.");

        OffsetRTV = Device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE handleRTV{
            HeapRTV->GetCPUDescriptorHandleForHeapStart()};
        for (int i = 0; i < BUFFERCOUNT; i++)
        {
            hr = SwapChain->GetBuffer(i, IID_PPV_ARGS(RTs[i].GetAddressOf()));
            assert(SUCCEEDED(hr) && "Failed to get swap chain buffer.");

            Device->CreateRenderTargetView(RTs[i].Get(), nullptr, handleRTV);
            handleRTV.Offset(1, OffsetRTV);
        }
    }
#pragma endregion
}
} // namespace

namespace Acrylic::D3D12
{
//==============================================================================
// External Function
//==============================================================================
void Init()
{
    InitGraphicsPipeline();

    br = DirectX::XMVerifyCPUSupport();
    assert(br && "CPU doesn't support SSE2 instructions.");

    LOG_INFO("Feature Support: SSE2.");
    LOG_INFO("Selected GPU: {}.", GPUName);
    LOG_INFO("Acrylic::D3D12::Init() succeeded.");
}

void WaitForSwapChain()
{
    WaitForSingleObject(EventSwapChain, 1000);
}

void Resize()
{
    // Acrylic::Frame::WaitForGPU();

    // Release the resources holding references to the swap chain (requirement
    // of IDXGISwapChain::ResizeBuffers)
    for (int i = 0; i < BUFFERCOUNT; i++)
    {
        RTs[i].Reset();
    }

    // Resize SwapChain buffers.
    DXGI_SWAP_CHAIN_DESC1 desc{};
    SwapChain->GetDesc1(&desc);
    hr = SwapChain->ResizeBuffers(BUFFERCOUNT,
                                  Acrylic::Window::GetWidth(),
                                  Acrylic::Window::GetHeight(),
                                  desc.Format,
                                  desc.Flags);
    assert(SUCCEEDED(hr) && "Failed to resize SwapChain buffers.");

    // Recreate RTVs for the new buffers.
    CD3DX12_CPU_DESCRIPTOR_HANDLE handleRTV{
        HeapRTV->GetCPUDescriptorHandleForHeapStart()};
    for (int i = 0; i < BUFFERCOUNT; i++)
    {
        hr = SwapChain->GetBuffer(i, IID_PPV_ARGS(RTs[i].GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to get swap chain buffer.");

        Device->CreateRenderTargetView(RTs[i].Get(), nullptr, handleRTV);
        handleRTV.Offset(1, OffsetRTV);
    }
}

void PresentSync()
{
    // Present w/ V-Sync.
    hr = SwapChain->Present(1, 0);
    assert(SUCCEEDED(hr) && "Failed to present swap chain frame.");
}

void PresentTear()
{
    // Present w/o V-Sync.
    hr = SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    assert(SUCCEEDED(hr) && "Failed to present swap chain frame.");
}
// void Exit()
// {
//     waitForGPU();
//     CloseHandle(EventFence);
//     CloseHandle(EventSwapChain);
// }

//==============================================================================
// Accessors
//==============================================================================
auto GetDevice() -> ID3D12Device9*
{
    return Device.Get();
}
auto GetMemAllocator() -> D3D12MA::Allocator*
{
    return MemAllocator.Get();
}
auto GetCmdQueue() -> ID3D12CommandQueue*
{
    return CmdQueue.Get();
}
auto GetCmdList() -> ID3D12GraphicsCommandList6*
{
    return CmdList.Get();
}
auto GetCurrentRT() -> ID3D12Resource*
{
    return RTs[SwapChain->GetCurrentBackBufferIndex()].Get();
}
auto GetCurrentRTV() -> D3D12_CPU_DESCRIPTOR_HANDLE
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE{
        HeapRTV->GetCPUDescriptorHandleForHeapStart(),
        static_cast<int>(SwapChain->GetCurrentBackBufferIndex()),
        OffsetRTV};
}
} // namespace Acrylic::D3D12