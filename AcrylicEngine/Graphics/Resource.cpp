#include "Resource.hpp"
#include "Asset.hpp"
#include "Util.hpp"
#include "D3D12.hpp"
#include "Log.hpp"
#include "DescriptorPool.hpp"

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
HANDLE Event{};
ComPtr<ID3D12Fence1> Fence{};

vector<ComPtr<D3D12MA::Allocation>> AllocUploads{};
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
    { // Create CmdAlctr.
        HR = Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CmdAlctr.GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create command allocator.");
    }
    { // Create CmdList.
        HR = Device->CreateCommandList1(0,
                                        D3D12_COMMAND_LIST_TYPE_DIRECT,
                                        D3D12_COMMAND_LIST_FLAG_NONE,
                                        IID_PPV_ARGS(CmdList.GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create command list.");
    }
    { // Create Fence and Event.
        HR = Device->CreateFence(0,
                                 D3D12_FENCE_FLAG_NONE,
                                 IID_PPV_ARGS(Fence.GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create fence.");

        Event = CreateEventW(nullptr, false, false, nullptr);
        assert(Event != nullptr && "Failed to create frame event.");
    }
}

void AllocateBuffer(const vector<Byte>& allocCPU,
                    D3D12MA::Allocation** ppAllocGPU,
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
                                  ppAllocGPU,
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
    cmdList->CopyBufferRegion((*ppAllocGPU)->GetResource(),
                              0,
                              (*ppAllocUpload)->GetResource(),
                              0,
                              allocCPU.size());

    // Transition to read state
    CD3DX12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition((*ppAllocGPU)->GetResource(),
                                             D3D12_RESOURCE_STATE_COPY_DEST,
                                             D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &barrier);
}

void AllocateTexture(const vector<Byte>& allocCPU,
                     D3D12MA::Allocation** ppAllocGPU,
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
                                  ppAllocGPU,
                                  IID_NULL,
                                  nullptr);
    assert(SUCCEEDED(hr) && "Failed to create a default texture.");

    // Create a temporary upload buffer
    const auto uploadBufferSize =
        GetRequiredIntermediateSize((*ppAllocGPU)->GetResource(), 0, 1);
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
                       (*ppAllocGPU)->GetResource(),
                       (*ppAllocUpload)->GetResource(),
                       0,
                       0,
                       1,
                       &subresource);

    // Transition to read state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        (*ppAllocGPU)->GetResource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}

} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Engine::Resource
{
//==============================================================================
// External Function
//==============================================================================
void Init()
{
    Device   = Acrylic::Engine::D3D12::GetPtrDevice();
    AlctrGPU = Acrylic::Engine::D3D12::GetPtrAlctrGPU();

    InitCopyEngine();

    LOG_INFO("Acrylic::Engine::Resource::Init() succeeded.");
}

void BeginAllocate()
{
    HR = CmdAlctr->Reset();
    assert(SUCCEEDED(HR) && "Failed to reset command allocator.");

    HR = CmdList->Reset(CmdAlctr.Get(), nullptr);
    assert(SUCCEEDED(HR) && "Failed to reset command list.");
}

void AllocateAll(Acrylic::Engine::DescriptorPoolCSU& poolSRV)
{
    auto& viewMeshes     = Acrylic::Engine::Asset::GetRefViewMeshes();
    auto& viewMaterials  = Acrylic::Engine::Asset::GetRefViewMaterials();
    auto& allocMeshes    = GetRefAllocMeshes();
    auto& allocMaterials = GetRefAllocMaterials();

    // Processing meshes
    for (const auto& viewMesh : viewMeshes)
    {
        AllocMesh allocMesh{};

        { // Vertex Buffer
            vector<Byte> viewFile{};

            BR = Acrylic::Engine::Util::LoadBinary(viewMesh.VB.Path, viewFile);
            assert(BR && "Failed to load binary file.");

            U32 length = viewMesh.VB.Length == 0
                             ? viewFile.size() - viewMesh.VB.Offset
                             : viewMesh.VB.Length;
            vector<Byte> allocCPU{viewFile.begin() + viewMesh.VB.Offset,
                                  viewFile.begin() + viewMesh.VB.Offset +
                                      length};
            AllocUploads.emplace_back();

            AllocateBuffer(allocCPU,
                           allocMesh.VB.AllocGPU.GetAddressOf(),
                           AllocUploads.back().GetAddressOf(),
                           CmdList.Get(),
                           AlctrGPU);

            allocMesh.VBV.BufferLocation =
                allocMesh.VB.AllocGPU->GetResource()->GetGPUVirtualAddress();
            allocMesh.VBV.StrideInBytes = sizeof(F32) * 5;
            allocMesh.VBV.SizeInBytes   = allocCPU.size();
        }

        { // Index Buffer
            vector<Byte> viewFile{};

            BR = Acrylic::Engine::Util::LoadBinary(viewMesh.IB.Path, viewFile);
            assert(BR && "Failed to load binary file.");

            U32 length = viewMesh.IB.Length == 0
                             ? viewFile.size() - viewMesh.IB.Offset
                             : viewMesh.IB.Length;
            vector<Byte> allocCPU{viewFile.begin() + viewMesh.IB.Offset,
                                  viewFile.begin() + viewMesh.IB.Offset +
                                      length};

            AllocUploads.emplace_back();

            AllocateBuffer(allocCPU,
                           allocMesh.IB.AllocGPU.GetAddressOf(),
                           AllocUploads.back().GetAddressOf(),
                           CmdList.Get(),
                           AlctrGPU);

            allocMesh.IBV.BufferLocation =
                allocMesh.IB.AllocGPU->GetResource()->GetGPUVirtualAddress();
            allocMesh.IBV.Format      = DXGI_FORMAT_R16_UINT;
            allocMesh.IBV.SizeInBytes = allocCPU.size();
        }

        allocMeshes.emplace_back(allocMesh);
    }

    // Processing materials
    for (const auto& viewMaterial : viewMaterials)
    {
        AllocMaterial allocMaterial{};

        if (viewMaterial.TexBaseColor.has_value())
        {
            allocMaterial.TexBaseColor = AllocTexture{};

            vector<Byte> viewFile{};
            int width{};
            int height{};

            BR = Acrylic::Engine::Util::LoadImage(
                viewMaterial.TexBaseColor->Path,
                viewFile,
                width,
                height);
            assert(BR && "Failed to load binary file.");

            U32 length = viewMaterial.TexBaseColor->Length == 0
                             ? viewFile.size()
                             : viewMaterial.TexBaseColor->Length;
            vector<Byte> allocCPU{
                viewFile.begin() + viewMaterial.TexBaseColor->Offset,
                viewFile.begin() + viewMaterial.TexBaseColor->Offset + length};

            AllocUploads.emplace_back();

            AllocateTexture(allocCPU,
                            allocMaterial.TexBaseColor->AllocGPU.GetAddressOf(),
                            AllocUploads.back().GetAddressOf(),
                            CmdList.Get(),
                            AlctrGPU,
                            width,
                            height);

            // Create SRV for the texture.
            allocMaterial.TexBaseColor->DescriptorIndex =
                poolSRV.AcquireIndex();
            Acrylic::Engine::Util::CreateSRV2D(
                Device,
                allocMaterial.TexBaseColor->AllocGPU->GetResource(),
                DXGI_FORMAT_R8G8B8A8_UNORM,
                1,
                poolSRV.Index2HandleCPU(
                    allocMaterial.TexBaseColor->DescriptorIndex));
        }
        if (viewMaterial.TexNormal.has_value())
        {
            allocMaterial.TexNormal = AllocTexture{};

            vector<Byte> viewFile{};
            int width{};
            int height{};

            BR = Acrylic::Engine::Util::LoadImage(viewMaterial.TexNormal->Path,
                                                  viewFile,
                                                  width,
                                                  height);
            assert(BR && "Failed to load binary file.");

            U32 length = viewMaterial.TexNormal->Length == 0
                             ? viewFile.size()
                             : viewMaterial.TexNormal->Length;
            vector<Byte> allocCPU{
                viewFile.begin() + viewMaterial.TexNormal->Offset,
                viewFile.begin() + viewMaterial.TexNormal->Offset + length};

            AllocUploads.emplace_back();

            AllocateTexture(allocCPU,
                            allocMaterial.TexNormal->AllocGPU.GetAddressOf(),
                            AllocUploads.back().GetAddressOf(),
                            CmdList.Get(),
                            AlctrGPU,
                            width,
                            height);

            // Create SRV for the texture.
            allocMaterial.TexNormal->DescriptorIndex = poolSRV.AcquireIndex();
            Acrylic::Engine::Util::CreateSRV2D(
                Device,
                allocMaterial.TexNormal->AllocGPU->GetResource(),
                DXGI_FORMAT_R8G8B8A8_UNORM,
                1,
                poolSRV.Index2HandleCPU(
                    allocMaterial.TexNormal->DescriptorIndex));
        }
        if (viewMaterial.TexARM.has_value())
        {
            allocMaterial.TexARM = AllocTexture{};

            vector<Byte> viewFile{};
            int width{};
            int height{};

            BR = Acrylic::Engine::Util::LoadImage(viewMaterial.TexARM->Path,
                                                  viewFile,
                                                  width,
                                                  height);
            assert(BR && "Failed to load binary file.");

            U32 length = viewMaterial.TexARM->Length == 0
                             ? viewFile.size()
                             : viewMaterial.TexARM->Length;
            vector<Byte> allocCPU{
                viewFile.begin() + viewMaterial.TexARM->Offset,
                viewFile.begin() + viewMaterial.TexARM->Offset + length};

            AllocUploads.emplace_back();

            AllocateTexture(allocCPU,
                            allocMaterial.TexARM->AllocGPU.GetAddressOf(),
                            AllocUploads.back().GetAddressOf(),
                            CmdList.Get(),
                            AlctrGPU,
                            width,
                            height);

            // Create SRV for the texture.
            allocMaterial.TexARM->DescriptorIndex = poolSRV.AcquireIndex();
            Acrylic::Engine::Util::CreateSRV2D(
                Device,
                allocMaterial.TexARM->AllocGPU->GetResource(),
                DXGI_FORMAT_R8G8B8A8_UNORM,
                1,
                poolSRV.Index2HandleCPU(allocMaterial.TexARM->DescriptorIndex));
        }
        if (viewMaterial.TexEmissive.has_value())
        {
            allocMaterial.TexEmissive = AllocTexture{};

            vector<Byte> viewFile{};
            int width{};
            int height{};

            BR =
                Acrylic::Engine::Util::LoadImage(viewMaterial.TexEmissive->Path,
                                                 viewFile,
                                                 width,
                                                 height);
            assert(BR && "Failed to load binary file.");

            U32 length = viewMaterial.TexEmissive->Length == 0
                             ? viewFile.size()
                             : viewMaterial.TexEmissive->Length;
            vector<Byte> allocCPU{
                viewFile.begin() + viewMaterial.TexEmissive->Offset,
                viewFile.begin() + viewMaterial.TexEmissive->Offset + length};

            AllocUploads.emplace_back();

            AllocateTexture(allocCPU,
                            allocMaterial.TexEmissive->AllocGPU.GetAddressOf(),
                            AllocUploads.back().GetAddressOf(),
                            CmdList.Get(),
                            AlctrGPU,
                            width,
                            height);

            // Create SRV for the texture.
            allocMaterial.TexEmissive->DescriptorIndex = poolSRV.AcquireIndex();
            Acrylic::Engine::Util::CreateSRV2D(
                Device,
                allocMaterial.TexEmissive->AllocGPU->GetResource(),
                DXGI_FORMAT_R8G8B8A8_UNORM,
                1,
                poolSRV.Index2HandleCPU(
                    allocMaterial.TexEmissive->DescriptorIndex));
        }

        allocMaterials.emplace_back(allocMaterial);
    }
}

auto EndAllocate() -> future<void>
{
    HR = CmdList->Close();
    assert(SUCCEEDED(HR) && "Failed to close command list.");

    vector<ID3D12CommandList*> cmdLists{CmdList.Get()};
    CmdQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());

    HR = CmdQueue->Signal(Fence.Get(), 1);
    assert(SUCCEEDED(HR) && "Failed to signal command queue.");

    HR = Fence->SetEventOnCompletion(1, Event);
    assert(SUCCEEDED(HR) && "Failed to set event on completion.");

    future<void> future = std::async(std::launch::async, [=]() -> void {
        WaitForSingleObject(Event, INFINITE);
        AllocUploads.clear();
    });

    return future;
}
} // namespace Acrylic::Engine::Resource
#pragma endregion