#include "Scene.hpp"
#include "D3D12.hpp"
#include "FrameResource.hpp"
#include "Log.hpp"
#include "Util.hpp"
#include "Window.hpp"

#include <D3D12MemAlloc.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d12.h>
#include <d3dx12/d3dx12.h>

#include <windows.h>
#include <wrl/client.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
//==============================================================================
// Variable
//==============================================================================
struct Vertex
{
    DirectX::XMFLOAT3 Position{};
    DirectX::XMFLOAT2 UV{};
};

struct ConstantBuffer
{
    DirectX::XMFLOAT4 Offset{};
    std::array<float, 60> padding{};
};
static_assert((sizeof(ConstantBuffer) % 256) == 0,
              "Constant Buffer size must be 256-byte aligned");

HRESULT hr{};
bool br{};

ID3D12Device9* Device;
D3D12MA::Allocator* MemAllocator;
ID3D12CommandQueue* CmdQueue;
ID3D12GraphicsCommandList6* CmdList;
ID3D12CommandAllocator* CmdAllocator;

ComPtr<ID3D12RootSignature> RS{};
ComPtr<ID3D12PipelineState> PSO{};

ComPtr<ID3D12DescriptorHeap> HeapCSU{};
std::uint32_t OffsetCSU{0};
D3D12_VERTEX_BUFFER_VIEW VBV{};
ComPtr<D3D12MA::Allocation> AllocVB{};
ComPtr<D3D12MA::Allocation> AllocCB{};
ComPtr<D3D12MA::Allocation> AllocTexture{};
ConstantBuffer CBData{};
void* pCBMemBegin{};

//==============================================================================
// Function
//=============================================================================
void CreateRS()
{
    std::array<CD3DX12_DESCRIPTOR_RANGE1, 2> ranges{
        CD3DX12_DESCRIPTOR_RANGE1{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                                  1,
                                  0,
                                  0,
                                  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC},
        CD3DX12_DESCRIPTOR_RANGE1{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                                  1,
                                  0,
                                  0,
                                  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC}};

    std::array<CD3DX12_ROOT_PARAMETER1, 2> rootParameters{};
    rootParameters[0].InitAsDescriptorTable(1,
                                            &ranges[0],
                                            D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1,
                                            &ranges[1],
                                            D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MipLODBias       = 0;
    sampler.MaxAnisotropy    = 0;
    sampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD           = 0.0F;
    sampler.MaxLOD           = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister   = 0;
    sampler.RegisterSpace    = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Allow input layout and deny uneccessary access to certain pipeline
    // stages.
    D3D12_ROOT_SIGNATURE_FLAGS flagRS =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC descRS{};
    descRS.Init_1_1(rootParameters.size(),
                    rootParameters.data(),
                    1,
                    &sampler,
                    flagRS);

    ComPtr<ID3DBlob> signature{};
    ComPtr<ID3DBlob> error{};

    hr = D3DX12SerializeVersionedRootSignature(&descRS,
                                               D3D_ROOT_SIGNATURE_VERSION_1_1,
                                               signature.GetAddressOf(),
                                               error.GetAddressOf());
    assert(SUCCEEDED(hr) && "Failed to serialize versioned root signature.");

    hr = Device->CreateRootSignature(0,
                                     signature->GetBufferPointer(),
                                     signature->GetBufferSize(),
                                     IID_PPV_ARGS(RS.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create versioned root signature.");
}

void CreatePSO()
{

    std::vector<std::byte> binVS{};
    br = Acrylic::Util::LoadBinary(LR"(Shader\HelloCB.vs.bin)", binVS);
    assert(br && "Failed to load vertex shader.");

    std::vector<std::byte> binPS{};
    br = Acrylic::Util::LoadBinary(LR"(Shader\HelloCB.ps.bin)", binPS);
    assert(br && "Failed to load pixel shader.");

    constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 2> descsInputElement{
        D3D12_INPUT_ELEMENT_DESC{"POSITION",
                                 0,
                                 DXGI_FORMAT_R32G32B32_FLOAT,
                                 0,
                                 0,
                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                 0},
        D3D12_INPUT_ELEMENT_DESC{"TEXCOORD",
                                 0,
                                 DXGI_FORMAT_R32G32_FLOAT,
                                 0,
                                 12,
                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPSO{};
    descPSO.InputLayout    = D3D12_INPUT_LAYOUT_DESC{descsInputElement.data(),
                                                  descsInputElement.size()};
    descPSO.pRootSignature = RS.Get();
    descPSO.VS = CD3DX12_SHADER_BYTECODE{binVS.data(), binVS.size()};
    descPSO.PS = CD3DX12_SHADER_BYTECODE{binPS.data(), binPS.size()};
    descPSO.RasterizerState = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
    descPSO.BlendState      = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
    descPSO.DepthStencilState.DepthEnable   = false;
    descPSO.DepthStencilState.StencilEnable = false;
    descPSO.SampleMask            = std::numeric_limits<std::uint32_t>::max();
    descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPSO.NumRenderTargets      = 1;
    descPSO.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
    descPSO.SampleDesc.Count      = 1;

    hr = Device->CreateGraphicsPipelineState(&descPSO,
                                             IID_PPV_ARGS(PSO.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create pipeline state.");
}

void CreateVB()
{
    float aspectRatio{static_cast<float>(Acrylic::Window::GetWidth())
                      / static_cast<float>(Acrylic::Window::GetHeight())};

    std::array<Vertex, 3> dataVB{
        Vertex{{0.0F, 0.5F * aspectRatio, 0.0F}, {0.5F, 0.0F}},
        Vertex{{0.5F, -0.5F * aspectRatio, 0.0F}, {1.0F, 1.0F}},
        Vertex{{-0.5F, -0.5F * aspectRatio, 0.0F}, {0.0F, 1.0F}}};
    auto sizeVB = static_cast<std::uint32_t>(sizeof(dataVB));

    D3D12MA::CALLOCATION_DESC descAlloc{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    CD3DX12_RESOURCE_DESC descRes = CD3DX12_RESOURCE_DESC::Buffer(sizeVB);

    hr = MemAllocator->CreateResource(&descAlloc,
                                      &descRes,
                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                      nullptr,
                                      AllocVB.GetAddressOf(),
                                      IID_NULL,
                                      nullptr);
    assert(SUCCEEDED(hr) && "Failed to create vertex buffer.");

    void* pMemBegin{};
    CD3DX12_RANGE readRange{0, 0};
    hr = AllocVB->GetResource()->Map(0, &readRange, &pMemBegin);
    assert(SUCCEEDED(hr) && "Failed to map vertex buffer.");

    memcpy(pMemBegin, dataVB.data(), sizeVB);
    AllocVB->GetResource()->Unmap(0, nullptr);

    VBV.BufferLocation = AllocVB->GetResource()->GetGPUVirtualAddress();
    VBV.StrideInBytes  = sizeof(Vertex);
    VBV.SizeInBytes    = sizeVB;
}

void CreateCB()
{
    auto sizeCB = sizeof(ConstantBuffer);
    D3D12MA::CALLOCATION_DESC descAlloc{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    CD3DX12_RESOURCE_DESC descRes = CD3DX12_RESOURCE_DESC::Buffer(sizeCB);

    hr = MemAllocator->CreateResource(&descAlloc,
                                      &descRes,
                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                      nullptr,
                                      AllocCB.GetAddressOf(),
                                      IID_NULL,
                                      nullptr);
    assert(SUCCEEDED(hr) && "Failed to create constant buffer.");

    D3D12_CONSTANT_BUFFER_VIEW_DESC descCBV{};
    descCBV.BufferLocation = AllocCB->GetResource()->GetGPUVirtualAddress();
    descCBV.SizeInBytes    = sizeCB;
    Device->CreateConstantBufferView(
        &descCBV,
        HeapCSU->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RANGE readRange{0, 0};
    hr = AllocCB->GetResource()->Map(0, &readRange, &pCBMemBegin);
    assert(SUCCEEDED(hr) && "Failed to map constant buffer.");
    memcpy(pCBMemBegin, &CBData, sizeof(CBData));
}

void CreateSR()
{
    hr = CmdAllocator->Reset();
    assert(SUCCEEDED(hr) && "Failed to reset command allocator.");

    hr = CmdList->Reset(CmdAllocator, PSO.Get());
    assert(SUCCEEDED(hr) && "Failed to reset command list.");

    D3D12MA::CALLOCATION_DESC descAllocDefault{
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    D3D12_RESOURCE_DESC descResDefault{};
    descResDefault.MipLevels          = 1;
    descResDefault.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    descResDefault.Width              = Acrylic::Scene::TEXTUREWIDTH;
    descResDefault.Height             = Acrylic::Scene::TEXTUREHEIGHT;
    descResDefault.Flags              = D3D12_RESOURCE_FLAG_NONE;
    descResDefault.DepthOrArraySize   = 1;
    descResDefault.SampleDesc.Count   = 1;
    descResDefault.SampleDesc.Quality = 0;
    descResDefault.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    hr = MemAllocator->CreateResource(&descAllocDefault,
                                      &descResDefault,
                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                      nullptr,
                                      AllocTexture.GetAddressOf(),
                                      IID_NULL,
                                      nullptr);
    assert(SUCCEEDED(hr) && "Failed to create texture resource.");

    ComPtr<D3D12MA::Allocation> allocUpload{};
    const auto uploadBufferSize =
        GetRequiredIntermediateSize(AllocTexture->GetResource(), 0, 1);
    D3D12MA::CALLOCATION_DESC descAllocUpload{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    auto descResUpload = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = MemAllocator->CreateResource(&descAllocUpload,
                                      &descResUpload,
                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                      nullptr,
                                      allocUpload.GetAddressOf(),
                                      IID_NULL,
                                      nullptr);
    assert(SUCCEEDED(hr) && "Failed to create texture upload resource.");

    std::vector<std::byte> texture{};
    br = Acrylic::Util::LoadImage(
        LR"(Asset\Texture\marble69\marble69_basecolor.png)",
        texture);
    assert(br && "Failed to load texture image.");

    D3D12_SUBRESOURCE_DATA textureData{};
    textureData.pData = texture.data();
    textureData.RowPitch =
        Acrylic::Scene::TEXTUREWIDTH * Acrylic::Scene::TEXELSIZE;
    textureData.SlicePitch =
        textureData.RowPitch * Acrylic::Scene::TEXTUREHEIGHT;

    UpdateSubresources(CmdList,
                       AllocTexture->GetResource(),
                       allocUpload->GetResource(),
                       0,
                       0,
                       1,
                       &textureData);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        AllocTexture->GetResource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    CmdList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
        HeapCSU->GetCPUDescriptorHandleForHeapStart(),
        1,
        OffsetCSU};

    D3D12_SHADER_RESOURCE_VIEW_DESC descSRV{};
    descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    descSRV.Format                  = descResDefault.Format;
    descSRV.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    descSRV.Texture2D.MipLevels     = 1;

    Device->CreateShaderResourceView(AllocTexture->GetResource(),
                                     &descSRV,
                                     handleSRV);

    hr = CmdList->Close();
    assert(SUCCEEDED(hr) && "Failed to close command list.");
    std::vector<ID3D12CommandList*> cmdLists{CmdList};
    CmdQueue->ExecuteCommandLists(static_cast<std::uint32_t>(cmdLists.size()),
                                  cmdLists.data());
    Acrylic::FrameResource::WaitForGPU();
}

void populateCmdList()
{
    auto currentRT  = Acrylic::D3D12::GetCurrentRT();
    auto currentRTV = Acrylic::D3D12::GetCurrentRTV();

    CD3DX12_VIEWPORT Viewport{0.0F,
                              0.0F,
                              static_cast<float>(Acrylic::Window::GetWidth()),
                              static_cast<float>(Acrylic::Window::GetHeight())};
    CD3DX12_RECT ScissorRect{0,
                             0,
                             static_cast<long>(Acrylic::Window::GetWidth()),
                             static_cast<long>(Acrylic::Window::GetHeight())};

    hr = CmdAllocator->Reset();
    assert(SUCCEEDED(hr) && "Failed to reset command allocator.");

    hr = CmdList->Reset(CmdAllocator, PSO.Get());
    assert(SUCCEEDED(hr) && "Failed to reset command list.");

    CmdList->SetGraphicsRootSignature(RS.Get());
    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->IASetVertexBuffers(0, 1, &VBV);

    std::vector<ID3D12DescriptorHeap*> heaps{HeapCSU.Get()};
    CmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

    CD3DX12_GPU_DESCRIPTOR_HANDLE HandleSRV{
        HeapCSU->GetGPUDescriptorHandleForHeapStart(),
        1,
        OffsetCSU};
    CmdList->SetGraphicsRootDescriptorTable(
        0,
        HeapCSU->GetGPUDescriptorHandleForHeapStart());
    CmdList->SetGraphicsRootDescriptorTable(1, HandleSRV);

    CD3DX12_RESOURCE_BARRIER p2r = CD3DX12_RESOURCE_BARRIER::Transition(
        currentRT,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &p2r);

    CmdList->OMSetRenderTargets(1, &currentRTV, false, nullptr);

    std::vector<float> clearColor{0.0F, 0.2F, 0.4F, 1.0F};
    CmdList->ClearRenderTargetView(currentRTV, clearColor.data(), 0, nullptr);
    CmdList->DrawInstanced(3, 1, 0, 0);

    CD3DX12_RESOURCE_BARRIER r2p =
        CD3DX12_RESOURCE_BARRIER::Transition(currentRT,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET,
                                             D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &r2p);

    hr = CmdList->Close();
    assert(SUCCEEDED(hr) && "Failed to close command list.");
}
} // namespace

namespace Acrylic::Scene
{
void Init()
{
    Device       = Acrylic::D3D12::GetDevice();
    MemAllocator = Acrylic::D3D12::GetMemAllocator();
    CmdQueue     = Acrylic::D3D12::GetCmdQueue();
    CmdList      = Acrylic::D3D12::GetCmdList();
    CmdAllocator = Acrylic::FrameResource::GetCurrentCA();

    D3D12_DESCRIPTOR_HEAP_DESC heapDescCSU{};
    heapDescCSU.NumDescriptors = 2;
    heapDescCSU.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDescCSU.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = Device->CreateDescriptorHeap(&heapDescCSU,
                                      IID_PPV_ARGS(HeapCSU.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create CSU descriptor heap.");

    OffsetCSU = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CreateRS();
    CreatePSO();
    CreateVB();
    CreateCB();
    CreateSR();

    LOG_INFO("Acrylic::Scene::Init() succeeded.");
}
void Update()
{
    const float translationSpeed = 0.005f;
    const float offsetBounds     = 1.25f;

    CBData.Offset.x += translationSpeed;
    if (CBData.Offset.x > offsetBounds)
    {
        CBData.Offset.x = -offsetBounds;
    }
    memcpy(pCBMemBegin, &CBData, sizeof(CBData));
}

void Render()
{
    populateCmdList();

    std::vector<ID3D12CommandList*> cmdLists{CmdList};
    CmdQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());

    Acrylic::D3D12::PresentSync();
}
} // namespace Acrylic::Scene