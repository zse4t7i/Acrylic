#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
// Return result for assertion.
HRESULT HR{};
bool BR{};

// External D3D12 Objects
ComPtr<IDXGIFactory6> Factory{};
ComPtr<IDXGIAdapter4> Adapter{};
ComPtr<ID3D12Device9> Device{};
ComPtr<ID3D12CommandQueue> CmdQueue{};
ComPtr<D3D12MA::Allocator> MemAlctr{};

// D3D12 Properties
string GPUName{};
U32 StrideRTV{};
U32 StrideCSU{};

// SwapChain-related Objects
ComPtr<IDXGISwapChain4> SwapChain{};
HANDLE EventBufferAvailable{};
ComPtr<ID3D12DescriptorHeap> HeapRTV{};
array<ComPtr<ID3D12Resource>, Acrylic::D3D12::BUFFERCOUNT> RTs{};

// Frame Objects
int FrameIndex{0};
HANDLE EventCmdExecuted{};
ComPtr<ID3D12Fence1> FrameFence{};
array<U64, Acrylic::D3D12::FRAMECOUNT> FrameFVs{0, 0};

//==============================================================================
// Internal Function
//==============================================================================
void InitGraphicsPipeline()
{
    ComPtr<ID3D12Debug5> debugLayer{};
    ComPtr<ID3D12InfoQueue> infoQueue{};

    U32 dxgiFactoryFlags{0};
#ifdef DEBUG
    HR = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to enable D3D12 debug layer.");
    debugLayer->EnableDebugLayer();
    debugLayer->SetEnableGPUBasedValidation(true);
    debugLayer->SetEnableAutoName(true);
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    HR = CreateDXGIFactory2(dxgiFactoryFlags,
                            IID_PPV_ARGS(Factory.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create DXGI factory.");

    HR = Factory->EnumAdapterByGpuPreference(
        0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(Adapter.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to find a dGPU.");

    HR = D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_1,
                           IID_PPV_ARGS(Device.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to find a dGPU that supports D3D12 "
                            "Feature Level 12_1.");

    { // Enable GPU-based validation.
#ifdef DEBUG
        HR = Device.As(&infoQueue);
        assert(SUCCEEDED(HR) && "Failed to query ID3D12InfoQueue.");
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

        vector<D3D12_MESSAGE_ID> denyIds{
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE};

        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = denyIds.size();
        filter.DenyList.pIDList = denyIds.data();

        infoQueue->AddStorageFilterEntries(&filter);
#endif
    }

    { // Create CmdQueue.
        D3D12_COMMAND_QUEUE_DESC descCQ{};
        descCQ.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        descCQ.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        HR = Device->CreateCommandQueue(&descCQ,
                                        IID_PPV_ARGS(CmdQueue.GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create command queue.");
    }
}

void InitSwapChain()
{
    { // Create SwapChain.
        DXGI_SWAP_CHAIN_DESC1 descSwapChain{};
        descSwapChain.BufferCount = Acrylic::D3D12::BUFFERCOUNT;
        descSwapChain.Width = Acrylic::Window::GetWidth();
        descSwapChain.Height = Acrylic::Window::GetHeight();
        descSwapChain.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        descSwapChain.SampleDesc.Count = 1;
        descSwapChain.Flags =
            DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING |
            DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        ComPtr<IDXGISwapChain1> swapChain{};
        HR = Factory->CreateSwapChainForHwnd(
            CmdQueue.Get(), Acrylic::Window::GetHWnd(), &descSwapChain, nullptr,
            nullptr, swapChain.GetAddressOf());
        assert(SUCCEEDED(HR) && "Failed to create swap chain.");
        HR = swapChain.As(&SwapChain);
        assert(SUCCEEDED(HR) && "Failed to query IDXGISwapChain4.");

        HR = SwapChain->SetMaximumFrameLatency(2);
        assert(SUCCEEDED(HR) && "Failed to SetMaximumFrameLatency(2).");
        EventBufferAvailable = SwapChain->GetFrameLatencyWaitableObject();

        Factory->MakeWindowAssociation(Acrylic::Window::GetHWnd(),
                                       DXGI_MWA_NO_ALT_ENTER);
    }

    { // Create RTs and RTV
        D3D12_DESCRIPTOR_HEAP_DESC descHeapRTV{};
        descHeapRTV.NumDescriptors = Acrylic::D3D12::BUFFERCOUNT;
        descHeapRTV.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        descHeapRTV.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        HR = Device->CreateDescriptorHeap(&descHeapRTV,
                                          IID_PPV_ARGS(HeapRTV.GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create RTV descriptor heap.");

        StrideRTV = Device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE handleRTV{
            HeapRTV->GetCPUDescriptorHandleForHeapStart()};
        for (int i = 0; i < Acrylic::D3D12::BUFFERCOUNT; i++)
        {
            HR = SwapChain->GetBuffer(i, IID_PPV_ARGS(RTs[i].GetAddressOf()));
            assert(SUCCEEDED(HR) && "Failed to get SwapChain buffer.");

            Device->CreateRenderTargetView(RTs[i].Get(), nullptr, handleRTV);
            handleRTV.Offset(1, StrideRTV);
        }
    }

    HR = Acrylic::D3D12::GetDevice()->CreateFence(
        FrameFVs[FrameIndex]++, D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(FrameFence.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create fence.");

    EventCmdExecuted = CreateEventW(nullptr, false, false, nullptr);
    assert(EventCmdExecuted != nullptr && "Failed to create frame event.");
}

void InitD3D12MA()
{
    D3D12MA::ALLOCATOR_DESC descD3D12MA{};
    descD3D12MA.Flags = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS;
    descD3D12MA.pDevice = Device.Get();
    descD3D12MA.pAdapter = Adapter.Get();

    HR = D3D12MA::CreateAllocator(&descD3D12MA, MemAlctr.GetAddressOf());
    assert(SUCCEEDED(HR) && "Failed to create D3D12 Memory Allocator.");
}

void QueryHardwareInfo()
{
    BR = DirectX::XMVerifyCPUSupport();
    assert(BR && "CPU doesn't support SSE2(x64)/NEON(A64) instructions.");

    LOG_INFO("CPU Feature: SSE2(x64)/NEON(A64).");

    DXGI_ADAPTER_DESC1 descAdapter{};
    Adapter->GetDesc1(&descAdapter);
    Acrylic::Util::UTF1628(descAdapter.Description, GPUName);
    LOG_INFO("GPU Name: {}.", GPUName);

    StrideCSU = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}
} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::D3D12
{
void Init()
{
    InitGraphicsPipeline();
    InitSwapChain();
    InitD3D12MA();

    QueryHardwareInfo();

    LOG_INFO("Acrylic::D3D12::Init() succeeded.");
}

void WaitForBufferAvailable()
{
    WaitForSingleObject(EventBufferAvailable, 1000);
}

void WaitForCmdExecuted()
{
    HR = Acrylic::D3D12::GetCmdQueue()->Signal(FrameFence.Get(),
                                               FrameFVs[FrameIndex]);
    assert(SUCCEEDED(HR) && "Failed to signal command queue.");

    HR = FrameFence->SetEventOnCompletion(FrameFVs[FrameIndex]++,
                                          EventCmdExecuted);
    assert(SUCCEEDED(HR) && "Failed to set event on completion.");
    WaitForSingleObject(EventCmdExecuted, INFINITE);
}

void WaitForFrameAvailable()
{
    const auto currentFV = FrameFVs[FrameIndex];

    HR = Acrylic::D3D12::GetCmdQueue()->Signal(FrameFence.Get(), currentFV);
    assert(SUCCEEDED(HR) && "Failed to signal command queue.");

    FrameIndex = (FrameIndex + 1) % FRAMECOUNT;

    if (FrameFence->GetCompletedValue() < FrameFVs[FrameIndex])
    {
        HR = FrameFence->SetEventOnCompletion(FrameFVs[FrameIndex],
                                              EventCmdExecuted);
        assert(SUCCEEDED(HR) && "Failed to set event on completion.");
        WaitForSingleObject(EventCmdExecuted, INFINITE);
    }

    FrameFVs[FrameIndex] = currentFV + 1;
}

void Resize()
{
    // Acrylic::D3D12::WaitForCmdExecuted();

    // Release the resources holding references to the SwapChain.
    // Requirement of IDXGISwapChain::ResizeBuffers.
    for (int i = 0; i < BUFFERCOUNT; i++)
    {
        RTs[i].Reset();
    }

    // Resize SwapChain buffers.
    DXGI_SWAP_CHAIN_DESC1 desc{};
    SwapChain->GetDesc1(&desc);
    HR = SwapChain->ResizeBuffers(BUFFERCOUNT, Acrylic::Window::GetWidth(),
                                  Acrylic::Window::GetHeight(), desc.Format,
                                  desc.Flags);
    assert(SUCCEEDED(HR) && "Failed to resize SwapChain buffers.");

    // Recreate RTVs for the new buffers.
    CD3DX12_CPU_DESCRIPTOR_HANDLE handleRTV{
        HeapRTV->GetCPUDescriptorHandleForHeapStart()};
    for (int i = 0; i < BUFFERCOUNT; i++)
    {
        HR = SwapChain->GetBuffer(i, IID_PPV_ARGS(RTs[i].GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to get swap chain buffer.");

        Device->CreateRenderTargetView(RTs[i].Get(), nullptr, handleRTV);
        handleRTV.Offset(1, StrideRTV);
    }
}

void PresentSync()
{
    // Present w/ V-Sync.
    HR = SwapChain->Present(1, 0);
    assert(SUCCEEDED(HR) && "Failed to present swap chain frame.");
}

void PresentTear()
{
    // Present w/o V-Sync.
    HR = SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    assert(SUCCEEDED(HR) && "Failed to present swap chain frame.");
}

// void Exit()
// {
//     WaitForCmdExecuted();
//     CloseHandle(EventCmdExecuted);
//     CloseHandle(EventBufferAvailable);
// }

//==============================================================================
// Accessors
//==============================================================================
auto GetDevice() -> ID3D12Device9 *
{
    return Device.Get();
}
auto GetCmdQueue() -> ID3D12CommandQueue *
{
    return CmdQueue.Get();
}
auto GetMemAllocator() -> D3D12MA::Allocator *
{
    return MemAlctr.Get();
}
auto GetCurrentRT() -> ID3D12Resource *
{
    return RTs[SwapChain->GetCurrentBackBufferIndex()].Get();
}
auto GetCurrentRTV() -> D3D12_CPU_DESCRIPTOR_HANDLE
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE{
        HeapRTV->GetCPUDescriptorHandleForHeapStart(),
        static_cast<int>(SwapChain->GetCurrentBackBufferIndex()), StrideRTV};
}
auto GetFrameIndex() -> int
{
    return FrameIndex;
}
auto GetStrideCSU() -> U32
{
    return StrideCSU;
}
} // namespace Acrylic::D3D12
#pragma endregion