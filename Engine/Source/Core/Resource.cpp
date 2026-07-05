#include "Resource.hpp"

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
ID3D12Device9* Device;
D3D12MA::Allocator* AlctrGPU;

// Internal D3D12 Objects
ComPtr<ID3D12CommandQueue> CmdQueue{};
ComPtr<ID3D12CommandAllocator> CmdAlctr{};
ComPtr<ID3D12GraphicsCommandList6> CmdList{};

//==============================================================================
// Internal Function
//==============================================================================
void InitCopyEngine()
{
    { // Create CmdQueue.
        D3D12_COMMAND_QUEUE_DESC descQueue{
            .Type{D3D12_COMMAND_LIST_TYPE_DIRECT}};

        HR = Device->CreateCommandQueue(&descQueue,
                                        IID_PPV_ARGS(CmdQueue.GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create command queue.");
    }

    HR = Acrylic::D3D12::GetPtrDevice()->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(CmdAlctr.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create command allocator.");

    HR = Acrylic::D3D12::GetPtrDevice()->CreateCommandList1(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        D3D12_COMMAND_LIST_FLAG_NONE,
        IID_PPV_ARGS(CmdList.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create command list.");
}

void UploadBuffer(const vector<Byte>& allocCPU,
                  D3D12MA::Allocation** ppAllocDefault,
                  D3D12MA::Allocation** ppAllocUpload,
                  ID3D12GraphicsCommandList* cmdList,
                  D3D12MA::Allocator* alctrGPU)
{
    HRESULT hr{};

    D3D12MA::CALLOCATION_DESC descDefault{
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};

    auto descBuffer = CD3DX12_RESOURCE_DESC::Buffer(allocCPU.size());

    // Create a default buffer
    hr = alctrGPU->CreateResource(&descDefault,
                                  &descBuffer,
                                  D3D12_RESOURCE_STATE_COMMON,
                                  nullptr,
                                  ppAllocDefault,
                                  IID_NULL,
                                  nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a default buffer.");

    // Create a temporary upload buffer
    hr = alctrGPU->CreateResource(&descUpload,
                                  &descBuffer,
                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                  nullptr,
                                  ppAllocUpload,
                                  IID_NULL,
                                  nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a upload buffer.");

    // Map and copy CPU data to upload buffer
    void* pUpload{};
    hr = (*ppAllocUpload)->GetResource()->Map(0, nullptr, &pUpload);
    assert(SUCCEEDED(hr) && "Failed to map upload buffer.");

    memcpy(pUpload, allocCPU.data(), allocCPU.size());
    (*ppAllocUpload)->GetResource()->Unmap(0, nullptr);

    // Record copy command
    cmdList->CopyBufferRegion((*ppAllocDefault)->GetResource(),
                              0,
                              (*ppAllocUpload)->GetResource(),
                              0,
                              allocCPU.size());

    // Transition to read state
    CD3DX12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition((*ppAllocDefault)->GetResource(),
                                             D3D12_RESOURCE_STATE_COPY_DEST,
                                             D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &barrier);
}

void UploadTexture(const vector<Byte>& allocCPU,
                   D3D12MA::Allocation** ppAllocDefault,
                   D3D12MA::Allocation** ppAllocUpload,
                   ID3D12GraphicsCommandList* cmdList,
                   D3D12MA::Allocator* alctrGPU,
                   int width,
                   int height)
{
    HRESULT hr{};

    D3D12MA::CALLOCATION_DESC descDefault{
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};

    auto descTexture =
        CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);

    // Create a default buffer
    hr = alctrGPU->CreateResource(&descDefault,
                                  &descTexture,
                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                  nullptr,
                                  ppAllocDefault,
                                  IID_NULL,
                                  nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a default texture.");

    // Create a temporary upload buffer
    const auto uploadBufferSize =
        GetRequiredIntermediateSize((*ppAllocDefault)->GetResource(), 0, 1);
    auto descBuffer = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = alctrGPU->CreateResource(&descUpload,
                                  &descBuffer,
                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                  nullptr,
                                  ppAllocUpload,
                                  IID_NULL,
                                  nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a upload texture.");

    // Upload subresource
    D3D12_SUBRESOURCE_DATA subresource{};
    subresource.pData      = allocCPU.data();
    subresource.RowPitch   = static_cast<I64>(width) * 4;
    subresource.SlicePitch = subresource.RowPitch * height;

    UpdateSubresources(cmdList,
                       (*ppAllocDefault)->GetResource(),
                       (*ppAllocUpload)->GetResource(),
                       0,
                       0,
                       1,
                       &subresource);

    // Transition to read state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        (*ppAllocDefault)->GetResource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}

} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Resource
{
//==============================================================================
// External Function
//==============================================================================
void Init()
{
    Device   = Acrylic::D3D12::GetPtrDevice();
    AlctrGPU = Acrylic::D3D12::GetPtrAlctrGPU();

    InitCopyEngine();

    LOG_INFO("Acrylic::Resource::Init() succeeded.");
}
} // namespace Acrylic::Resource
#pragma endregion