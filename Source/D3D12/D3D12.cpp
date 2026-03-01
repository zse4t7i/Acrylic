#include "D3D12.hpp"
#include "Log.hpp"
#include "Util.hpp"

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
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace DirectX;
using namespace Acrylic::Util;
using Microsoft::WRL::ComPtr;

namespace
{
HRESULT hr{};
bool br{};

HWND HWnd{};

int Width{1920};
int Height{1200};
float AspectRatio{static_cast<float>(Width) / static_cast<float>(Height)};
CD3DX12_VIEWPORT Viewport{
    0.0F, 0.0F, static_cast<float>(Width), static_cast<float>(Height)};
CD3DX12_RECT ScissorRect{
    0, 0, static_cast<long>(Width), static_cast<long>(Height)};

ComPtr<ID3D12Device9> Device{};
ComPtr<D3D12MA::Allocator> MemAllocator{};
ComPtr<ID3D12CommandQueue> CmdQueue{};
ComPtr<ID3D12GraphicsCommandList6> CmdList{};
std::array<ComPtr<ID3D12CommandAllocator>, 2> FrameCAs{};

ComPtr<ID3D12Fence1> Fence{};
HANDLE EventFence{};
std::array<std::uint64_t, 2> FrameFVs{};
int IndexFrame{0};
ComPtr<IDXGISwapChain4> SwapChain{};

std::array<ComPtr<ID3D12Resource>, 3> RTs{};
ComPtr<ID3D12DescriptorHeap> HeapRTV{};
std::uint32_t OffsetRTV{0};

ComPtr<ID3D12PipelineState> PSO{};
ComPtr<ID3D12RootSignature> RS{};

ComPtr<D3D12MA::Allocation> AllocVertex{};
D3D12_VERTEX_BUFFER_VIEW VBV{};
ComPtr<ID3D12DescriptorHeap> HeapCSU{};
std::uint32_t OffsetCSU{0};
ComPtr<D3D12MA::Allocation> AllocTexture{};
ComPtr<D3D12MA::Allocation> AllocCB{};
ConstantBuffer CBData{};
void* pCBMemBegin{};

void waitForGPU()
{
    hr = CmdQueue->Signal(Fence.Get(), FrameFVs[IndexFrame]);
    assert(SUCCEEDED(hr) && "Failed to signal command queue.");

    hr = Fence->SetEventOnCompletion(FrameFVs[IndexFrame]++, EventFence);
    assert(SUCCEEDED(hr) && "Failed to set event on completion.");
    WaitForSingleObject(EventFence, INFINITE);
}

void moveToNextFrame()
{
    const auto currentFV = FrameFVs[IndexFrame];

    hr = CmdQueue->Signal(Fence.Get(), currentFV);
    assert(SUCCEEDED(hr) && "Failed to signal command queue.");

    IndexFrame = (IndexFrame + 1) % 2;

    if (Fence->GetCompletedValue() < FrameFVs[IndexFrame])
    {
        hr = Fence->SetEventOnCompletion(FrameFVs[IndexFrame], EventFence);
        assert(SUCCEEDED(hr) && "Failed to set event on completion.");
        WaitForSingleObject(EventFence, INFINITE);
    }

    FrameFVs[IndexFrame] = currentFV + 1;
}

void initGraphicsPipeline()
{
    std::uint32_t dxgiFactoryFlags = 0;

#ifdef DEBUG
    ComPtr<ID3D12Debug> debugLayer{};
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to enable D3D12 debug layer.");
    debugLayer->EnableDebugLayer();
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ComPtr<IDXGIFactory6> factory{};
    hr = CreateDXGIFactory2(dxgiFactoryFlags,
                            IID_PPV_ARGS(factory.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create DXGI factory.");

    ComPtr<IDXGIAdapter1> adapter{};
    hr = factory->EnumAdapterByGpuPreference(
        0,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(adapter.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to find a dGPU.");

    DXGI_ADAPTER_DESC1 adapterDesc{};
    adapter->GetDesc1(&adapterDesc);
    UTF1628(adapterDesc.Description, Acrylic::D3D12::GPUName);

    hr = D3D12CreateDevice(adapter.Get(),
                           D3D_FEATURE_LEVEL_12_2,
                           IID_PPV_ARGS(Device.GetAddressOf()));
    assert(SUCCEEDED(hr)
           && "Failed to find a dGPU that supports D3D12 "
              "Feature Level 12_2.");

    D3D12MA::ALLOCATOR_DESC descAlloc{};
    descAlloc.Flags    = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS;
    descAlloc.pDevice  = Device.Get();
    descAlloc.pAdapter = adapter.Get();

    hr = D3D12MA::CreateAllocator(&descAlloc, MemAllocator.GetAddressOf());
    assert(SUCCEEDED(hr) && "Failed to create D3D12 Memory Allocator.");

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = Device->CreateCommandQueue(&queueDesc,
                                    IID_PPV_ARGS(CmdQueue.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create command queue.");

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.BufferCount      = 3;
    swapChainDesc.Width            = Width;
    swapChainDesc.Height           = Height;
    swapChainDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags            = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
                        | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    ComPtr<IDXGISwapChain1> swapChain{};
    hr = factory->CreateSwapChainForHwnd(CmdQueue.Get(),
                                         HWnd,
                                         &swapChainDesc,
                                         nullptr,
                                         nullptr,
                                         swapChain.GetAddressOf());
    assert(SUCCEEDED(hr) && "Failed to create swap chain.");
    hr = swapChain.As(&SwapChain);
    assert(SUCCEEDED(hr) && "Failed to query IDXGISwapChain4.");

    hr = SwapChain->SetMaximumFrameLatency(2);
    assert(SUCCEEDED(hr) && "Failed to SetMaximumFrameLatency(2).");
    Acrylic::D3D12::EventSwapChain = SwapChain->GetFrameLatencyWaitableObject();

    factory->MakeWindowAssociation(HWnd, DXGI_MWA_NO_ALT_ENTER);

    D3D12_DESCRIPTOR_HEAP_DESC heapDescRTV{};
    heapDescRTV.NumDescriptors = 3;
    heapDescRTV.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDescRTV.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = Device->CreateDescriptorHeap(&heapDescRTV,
                                      IID_PPV_ARGS(HeapRTV.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create RTV descriptor heap.");

    OffsetRTV = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    OffsetCSU = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDescCSU{};
    heapDescCSU.NumDescriptors = 2;
    heapDescCSU.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDescCSU.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = Device->CreateDescriptorHeap(&heapDescCSU,
                                      IID_PPV_ARGS(HeapCSU.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create SRV descriptor heap.");

    CD3DX12_CPU_DESCRIPTOR_HANDLE HandleRTV{
        HeapRTV->GetCPUDescriptorHandleForHeapStart()};
    for (int i = 0; i < 3; ++i)
    {
        hr = SwapChain->GetBuffer(i, IID_PPV_ARGS(RTs[i].GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to get swap chain buffer.");

        Device->CreateRenderTargetView(RTs[i].Get(), nullptr, HandleRTV);
        HandleRTV.Offset(1, OffsetRTV);
    }
    for (int i = 0; i < 2; ++i)
    {
        hr = Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(FrameCAs[i].GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to create command allocator.");
    }

    hr = Device->CreateCommandList1(0,
                                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    D3D12_COMMAND_LIST_FLAG_NONE,
                                    IID_PPV_ARGS(CmdList.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create command list.");

    hr = Device->CreateFence(FrameFVs[IndexFrame]++,
                             D3D12_FENCE_FLAG_NONE,
                             IID_PPV_ARGS(Fence.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create fence.");

    EventFence = CreateEventW(nullptr, false, false, nullptr);
    assert(EventFence != nullptr && "Failed to create fence event.");
}

void loadAsset()
{
#pragma region RS
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

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter                    = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU                  = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV                  = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW                  = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias                = 0;
        sampler.MaxAnisotropy             = 0;
        sampler.ComparisonFunc            = D3D12_COMPARISON_FUNC_NEVER;
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

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC descRootSign{};
        descRootSign.Init_1_1(static_cast<std::uint32_t>(rootParameters.size()),
                              rootParameters.data(),
                              1,
                              &sampler,
                              flagRS);

        ComPtr<ID3DBlob> signature{};
        ComPtr<ID3DBlob> error{};

        hr = D3DX12SerializeVersionedRootSignature(
            &descRootSign,
            D3D_ROOT_SIGNATURE_VERSION_1_1,
            signature.GetAddressOf(),
            error.GetAddressOf());
        assert(SUCCEEDED(hr)
               && "Failed to serialize versioned root signature.");

        hr = Device->CreateRootSignature(0,
                                         signature->GetBufferPointer(),
                                         signature->GetBufferSize(),
                                         IID_PPV_ARGS(RS.GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to create versioned root signature.");
    }
#pragma endregion

#pragma region PSO
    {
        std::vector<std::byte> binVS{};
        br = LoadBinary(LR"(Shader\HelloCB.vs.bin)", binVS);
        assert(br && "Failed to load vertex shader.");

        std::vector<std::byte> binPS{};
        br = LoadBinary(LR"(Shader\HelloCB.ps.bin)", binPS);
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
        descPSO.InputLayout = D3D12_INPUT_LAYOUT_DESC{descsInputElement.data(),
                                                      descsInputElement.size()};
        descPSO.pRootSignature = RS.Get();
        descPSO.VS = CD3DX12_SHADER_BYTECODE{binVS.data(), binVS.size()};
        descPSO.PS = CD3DX12_SHADER_BYTECODE{binPS.data(), binPS.size()};
        descPSO.RasterizerState = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
        descPSO.BlendState      = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
        descPSO.DepthStencilState.DepthEnable   = false;
        descPSO.DepthStencilState.StencilEnable = false;
        descPSO.SampleMask = std::numeric_limits<std::uint32_t>::max();
        descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        descPSO.NumRenderTargets      = 1;
        descPSO.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
        descPSO.SampleDesc.Count      = 1;

        hr = Device->CreateGraphicsPipelineState(
            &descPSO,
            IID_PPV_ARGS(PSO.GetAddressOf()));
        assert(SUCCEEDED(hr) && "Failed to create pipeline state.");
    }
#pragma endregion

#pragma region VB
    {
        std::array<Vertex, 3> vb{
            Vertex{{0.0F, 0.5F * AspectRatio, 0.0F}, {0.5F, 0.0F}},
            Vertex{{0.5F, -0.5F * AspectRatio, 0.0F}, {1.0F, 1.0F}},
            Vertex{{-0.5F, -0.5F * AspectRatio, 0.0F}, {0.0F, 1.0F}}};
        auto sizeVB = static_cast<std::uint32_t>(sizeof(vb));

        D3D12MA::CALLOCATION_DESC descAlloc{
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
        CD3DX12_RESOURCE_DESC descRes = CD3DX12_RESOURCE_DESC::Buffer(sizeVB);

        hr = MemAllocator->CreateResource(&descAlloc,
                                          &descRes,
                                          D3D12_RESOURCE_STATE_GENERIC_READ,
                                          nullptr,
                                          AllocVertex.GetAddressOf(),
                                          IID_NULL,
                                          nullptr);
        assert(SUCCEEDED(hr) && "Failed to create vertex buffer.");

        void* pMemBegin{};
        CD3DX12_RANGE readRange{0, 0};
        hr = AllocVertex->GetResource()->Map(0, &readRange, &pMemBegin);
        assert(SUCCEEDED(hr) && "Failed to map vertex buffer.");
        memcpy(pMemBegin, vb.data(), sizeVB);
        AllocVertex->GetResource()->Unmap(0, nullptr);

        VBV.BufferLocation = AllocVertex->GetResource()->GetGPUVirtualAddress();
        VBV.StrideInBytes  = sizeof(Vertex);
        VBV.SizeInBytes    = sizeVB;
    }
#pragma endregion

#pragma region CB
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
#pragma endregion

#pragma region Texture
    {
        hr = FrameCAs[IndexFrame]->Reset();
        assert(SUCCEEDED(hr) && "Failed to reset command allocator.");

        hr = CmdList->Reset(FrameCAs[IndexFrame].Get(), PSO.Get());
        assert(SUCCEEDED(hr) && "Failed to reset command list.");

        D3D12MA::CALLOCATION_DESC descAllocDefault{
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
        D3D12_RESOURCE_DESC descResDefault{};
        descResDefault.MipLevels          = 1;
        descResDefault.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        descResDefault.Width              = Acrylic::D3D12::TEXTUREWIDTH;
        descResDefault.Height             = Acrylic::D3D12::TEXTUREHEIGHT;
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
        br = LoadImage(LR"(Asset\Texture\marble69\marble69_basecolor.png)",
                       texture);
        assert(br && "Failed to load texture image.");

        D3D12_SUBRESOURCE_DATA textureData{};
        textureData.pData = texture.data();
        textureData.RowPitch =
            Acrylic::D3D12::TEXTUREWIDTH * Acrylic::D3D12::TEXELSIZE;
        textureData.SlicePitch =
            textureData.RowPitch * Acrylic::D3D12::TEXTUREHEIGHT;

        UpdateSubresources(CmdList.Get(),
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
        descSRV.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        descSRV.Format              = descResDefault.Format;
        descSRV.ViewDimension       = D3D12_SRV_DIMENSION_TEXTURE2D;
        descSRV.Texture2D.MipLevels = 1;

        Device->CreateShaderResourceView(AllocTexture->GetResource(),
                                         &descSRV,
                                         handleSRV);

        hr = CmdList->Close();
        assert(SUCCEEDED(hr) && "Failed to close command list.");
        std::vector<ID3D12CommandList*> cmdLists{CmdList.Get()};
        CmdQueue->ExecuteCommandLists(
            static_cast<std::uint32_t>(cmdLists.size()),
            cmdLists.data());
        waitForGPU();
    }
#pragma endregion
}

void populateCmdList()
{
    hr = FrameCAs[IndexFrame]->Reset();
    assert(SUCCEEDED(hr) && "Failed to reset command allocator.");

    hr = CmdList->Reset(FrameCAs[IndexFrame].Get(), PSO.Get());
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

    int indexBuffer = SwapChain->GetCurrentBackBufferIndex();
    //LOG_INFO("indexBuffer: {}, IndexFrame: {}.", indexBuffer, IndexFrame);

    CD3DX12_RESOURCE_BARRIER p2r = CD3DX12_RESOURCE_BARRIER::Transition(
        RTs[indexBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &p2r);

    CD3DX12_CPU_DESCRIPTOR_HANDLE HandleRTV{
        HeapRTV->GetCPUDescriptorHandleForHeapStart(),
        indexBuffer,
        OffsetRTV};
    CmdList->OMSetRenderTargets(1, &HandleRTV, false, nullptr);

    std::vector<float> clearColor{0.0F, 0.2F, 0.4F, 1.0F};
    CmdList->ClearRenderTargetView(HandleRTV, clearColor.data(), 0, nullptr);
    CmdList->DrawInstanced(3, 1, 0, 0);

    CD3DX12_RESOURCE_BARRIER r2p =
        CD3DX12_RESOURCE_BARRIER::Transition(RTs[indexBuffer].Get(),
                                             D3D12_RESOURCE_STATE_RENDER_TARGET,
                                             D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &r2p);

    hr = CmdList->Close();
    assert(SUCCEEDED(hr) && "Failed to close command list.");
}

auto CALLBACK WndProc(HWND hWnd,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam) -> LRESULT
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
} // namespace

namespace Acrylic::D3D12
{
std::string GPUName{};

HANDLE EventSwapChain{};

void Init(HINSTANCE hInst,
          int nShowCmd)
{
    bool br = DirectX::XMVerifyCPUSupport();
    assert(br && "CPU doesn't support AVX2 instructions.");
    LOG_INFO("Feature Support: AVX2.");

#pragma region Create Window
    {
        WNDCLASSEXW wcex{};
        wcex.cbSize        = sizeof(WNDCLASSEXW);
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = WndProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = 0;
        wcex.hInstance     = hInst;
        wcex.hIcon         = LoadIconW(hInst, IDI_APPLICATION);
        wcex.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszMenuName  = nullptr;
        wcex.lpszClassName = L"AcrylicMainWindowClass";
        wcex.hIconSm       = LoadIconW(hInst, IDI_APPLICATION);
        RegisterClassExW(&wcex);

        RECT rc{0, 0, Width, Height};
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);

        // This makes sure that in a multi-monitor setup
        // with different resolutions, get monitor info
        // returns correct dimensions
        SetProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        // Create a window.
        HWnd = CreateWindowExW(0,
                               L"AcrylicMainWindowClass",
                               L"Acrylic",
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               rc.right - rc.left,
                               rc.bottom - rc.top,
                               nullptr,
                               nullptr,
                               hInst,
                               nullptr);
    }
#pragma endregion

    initGraphicsPipeline();
    loadAsset();

    ShowWindow(HWnd, nShowCmd);

    LOG_INFO("Selected dGPU: {}.", GPUName);
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

    std::vector<ID3D12CommandList*> cmdLists{CmdList.Get()};
    CmdQueue->ExecuteCommandLists(static_cast<std::uint32_t>(cmdLists.size()),
                                  cmdLists.data());

    hr = SwapChain->Present(1, 0);
    // hr = SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    assert(SUCCEEDED(hr) && "Failed to present swap chain frame.");

    moveToNextFrame();
}

void Destroy()
{
    waitForGPU();
    CloseHandle(EventFence);
    CloseHandle(EventSwapChain);
}
} // namespace Acrylic::D3D12