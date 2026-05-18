#include "Scene.hpp"
#include "D3D12.hpp"
#include "Frame.hpp"
#include "Log.hpp"
#include "Util.hpp"
#include "Window.hpp"

#include <D3D12MemAlloc.h>
#include <DirectXMath.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <entt/entt.hpp>

#include <windows.h>
#include <wrl/client.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
//==============================================================================
// Variable
//==============================================================================
struct MemoryAlloc
{
    std::vector<std::byte> AllocCPU;
    ComPtr<D3D12MA::Allocation> AllocDefault;
    ComPtr<D3D12MA::Allocation> AllocUpload;
    int DescriptorHeapOffset{};
};

struct MemoryMesh
{
    MemoryAlloc VertexBuffer{};
    D3D12_VERTEX_BUFFER_VIEW VBV{};
    MemoryAlloc IndexBuffer{};
    D3D12_INDEX_BUFFER_VIEW IBV{};
};

struct MemoryMaterial
{
    // int TextureWidth{};
    // int TextureHeight{};
    MemoryAlloc ShaderVertex{};
    MemoryAlloc ShaderPixel{};
    std::optional<MemoryAlloc> TexBaseColor;
    std::optional<MemoryAlloc> TexNormal;
    std::optional<MemoryAlloc> TexARM;
    std::optional<MemoryAlloc> TexEmissive;
};

struct DiskView
{
    std::filesystem::path Path{};
    std::uint32_t Offset{0};
    std::uint32_t Length{0};
    // When Length==0, it means from Offset to the end of the file.
};

struct DiskMesh
{
    DiskView VertexBuffer{};
    DiskView IndexBuffer{};
};

struct DiskMaterial
{
    DiskView ShaderVertex{};
    DiskView ShaderPixel{};
    std::optional<DiskView> TexBaseColor;
    std::optional<DiskView> TexNormal;
    std::optional<DiskView> TexARM;
    std::optional<DiskView> TexEmissive;
};

struct ComTag
{
    std::string Name;
};

struct ComRenderable
{
    XMFLOAT3 Position{0.0F, 0.0F, 0.0F};
    XMFLOAT3 Rotation{0.0F, 0.0F, 0.0F};
    XMFLOAT3 Scale{1.0F, 1.0F, 1.0F};
    std::vector<std::uint32_t> MeshIndices;
    std::vector<std::uint32_t> MaterialIndices;
};

struct ComLight
{
    XMFLOAT3 Position{0.0F, 0.0F, 0.0F};
    XMFLOAT4 Color{1.0F, 1.0F, 1.0F, 1.0F};
    float Intensity{1.0F};
};

struct ComCamera
{
    XMFLOAT3 Position{0.0F, 0.0F, 0.0F};
    float FOV{45.0F};
    float AspectRatio{16.0F / 9.0F};
    float PlaneNear{0.1F};
    float PlaneFar{1000.0F};
};

struct Entity
{
    ComTag Tag;

    std::optional<ComRenderable> Renderable;
    std::optional<ComLight> Light;
    std::optional<ComCamera> Camera;
};

struct Scene
{
    std::string Name;

    std::vector<std::uint32_t> EntityIndices;
};

struct Project
{
    std::string Name;
    int ActiveSceneIndex{0};

    std::vector<Scene> Scenes;
    std::vector<Entity> Entities;
    std::vector<DiskMesh> DiskMeshes;
    std::vector<DiskMaterial> DiskMaterials;
};

struct ConstantBuffer
{
    XMFLOAT4X4 MVP{};
    XMFLOAT4 Color{};
};

HRESULT hr{};
bool br{};

ID3D12Device9* Device;
D3D12MA::Allocator* MemAllocator;
ID3D12CommandQueue* CmdQueue;
ID3D12GraphicsCommandList6* CmdList;

std::unique_ptr<Project> ProjectInstance{};
std::unique_ptr<entt::registry> ECSRegistryInstance{};

std::vector<MemoryMesh> MemoryMeshes{};
std::vector<MemoryMaterial> MemoryMaterials{};

ComPtr<ID3D12RootSignature> RS{};
ComPtr<ID3D12PipelineState> PSO{};

ComPtr<D3D12MA::Allocation> AllocCB{};
void* PointerCB{};

ComPtr<ID3D12DescriptorHeap> HeapCSU{};
std::uint32_t StrideCSU{};

//==============================================================================
// Function
//==============================================================================
void BuildDefaultScene()
{
    ProjectInstance = std::make_unique<Project>(Project{
        .Name{"PerspectiveProject"},
        .Scenes{Scene{.Name{"HelloAcrylic"}, .EntityIndices{0, 1, 2}}},
        .Entities{
            Entity{.Tag{.Name{"Cube"}},
                   .Renderable{
                       ComRenderable{.MeshIndices{0}, .MaterialIndices{0}}}},
            Entity{.Tag{.Name{"Light"}}, .Light{{.Position{4.0F, 4.0F, 4.0F}}}},
            Entity{.Tag{.Name{"Camera"}},
                   .Camera{{.Position{2.0F, 2.0F, 2.0F}}}}},
        .DiskMeshes{DiskMesh{
            .VertexBuffer{.Path{"Mesh/Cube.bin"}, .Offset{0}, .Length{480}},
            .IndexBuffer{
                .Path{"Mesh/Cube.bin"},
                .Offset{480},
                .Length{72},
            }}},
        .DiskMaterials{DiskMaterial{
            .ShaderVertex{.Path{"Shader/Cube.vs.bin"}, .Offset{0}, .Length{0}},
            .ShaderPixel{.Path{"Shader/Cube.ps.bin"}, .Offset{0}, .Length{0}},
            .TexBaseColor{
                {.Path{"Texture/Metal_BaseColor.png"}, .Offset{0}, .Length{0}}},
            .TexNormal{
                {.Path{"Texture/Metal_Normal.png"}, .Offset{0}, .Length{0}}},
            .TexARM{
                {.Path{"Texture/Metal_ARM.png"}, .Offset{0}, .Length{0}}}}}});
}

void InitScene()
{
    BuildDefaultScene();
}

void InitMemoryCPU()
{
    std::unordered_map<std::string, std::vector<std::byte>> loadedFiles{};

    // Pre-loading all resource files from resource folder.
    for (const std::string& directory : {"Mesh", "Texture", "Shader"})
    {
        if (!std::filesystem::exists(directory))
        {
            continue;
        }

        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(directory))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if (directory == "Shader" && entry.path().extension() == ".pdb")
            {
                continue;
            }

            const auto& filePath = entry.path();
            // TODO: Move fileContent out of the loop.
            std::vector<std::byte> fileContent{};

            if (filePath.extension() == ".bin")
            {
                br = Acrylic::Util::LoadBinary(filePath, fileContent);
                assert(br && "Failed to load binary file.");
            }
            if (filePath.extension() == ".png")
            {
                br = Acrylic::Util::LoadImage(filePath, fileContent);
                assert(br && "Failed to load image file.");
            }

            // Use filename as key
            loadedFiles[filePath.filename().string()] = fileContent;
        }
    }

    // Acclocating for meshes.
    for (const auto& diskMesh : ProjectInstance->DiskMeshes)
    {
        assert(diskMesh.VertexBuffer.Path == diskMesh.IndexBuffer.Path
               && "Vertex and index buffer must be in the same file.");

        const auto& fileContent =
            loadedFiles[diskMesh.VertexBuffer.Path.filename().string()];

        std::uint32_t offsetVertex = diskMesh.VertexBuffer.Offset;
        std::uint32_t lengthVertex = diskMesh.VertexBuffer.Length == 0
                                       ? fileContent.size() - offsetVertex
                                       : diskMesh.VertexBuffer.Length;
        std::uint32_t offsetIndex  = diskMesh.IndexBuffer.Offset;
        std::uint32_t lengthIndex  = diskMesh.IndexBuffer.Length == 0
                                       ? fileContent.size() - offsetIndex
                                       : diskMesh.IndexBuffer.Length;

        MemoryMeshes.emplace_back(MemoryMesh{
            .VertexBuffer{
                .AllocCPU{fileContent.begin() + offsetVertex,
                          fileContent.begin() + offsetVertex + lengthVertex}},
            .IndexBuffer{
                .AllocCPU{fileContent.begin() + offsetIndex,
                          fileContent.begin() + offsetIndex + lengthIndex}}});
    }

    int heapOffset{1};
    // Acclocating for materials.
    for (const auto& diskMaterial : ProjectInstance->DiskMaterials)
    {
        MemoryMaterial memoryMaterial{
            .ShaderVertex{
                .AllocCPU{loadedFiles[diskMaterial.ShaderVertex.Path.filename()
                                          .string()]}},
            .ShaderPixel{
                .AllocCPU{loadedFiles[diskMaterial.ShaderPixel.Path.filename()
                                          .string()]}}};

        if (diskMaterial.TexBaseColor.has_value())
        {
            memoryMaterial.TexBaseColor = MemoryAlloc{
                .AllocCPU{loadedFiles[diskMaterial.TexBaseColor->Path.filename()
                                          .string()]},
                .DescriptorHeapOffset{heapOffset++}};
        }
        if (diskMaterial.TexNormal.has_value())
        {
            memoryMaterial.TexNormal = MemoryAlloc{
                .AllocCPU{loadedFiles[diskMaterial.TexNormal->Path.filename()
                                          .string()]},
                .DescriptorHeapOffset{heapOffset++}};
        }
        if (diskMaterial.TexARM.has_value())
        {
            memoryMaterial.TexARM = MemoryAlloc{
                .AllocCPU{
                    loadedFiles[diskMaterial.TexARM->Path.filename().string()]},
                .DescriptorHeapOffset{heapOffset++}};
        }
        if (diskMaterial.TexEmissive.has_value())
        {
            memoryMaterial.TexEmissive = MemoryAlloc{
                .AllocCPU{loadedFiles[diskMaterial.TexEmissive->Path.filename()
                                          .string()]},
                .DescriptorHeapOffset{heapOffset++}};
        }
        MemoryMaterials.emplace_back(std::move(memoryMaterial));
    }
}

void InitMemoryGPU()
{
    hr = Acrylic::Frame::GetCurrentCA()->Reset();
    assert(SUCCEEDED(hr) && "Failed to reset command allocator.");

    hr = CmdList->Reset(Acrylic::Frame::GetCurrentCA(), PSO.Get());
    assert(SUCCEEDED(hr) && "Failed to reset command list.");

    // Processing BUFFERS (Vertex & Index)
    for (auto& memMesh : MemoryMeshes)
    {
        // Vertex Buffer
        Acrylic::Util::UploadBuffer(
            memMesh.VertexBuffer.AllocCPU,
            memMesh.VertexBuffer.AllocDefault.GetAddressOf(),
            memMesh.VertexBuffer.AllocUpload.GetAddressOf(),
            CmdList,
            MemAllocator);

        memMesh.VBV.BufferLocation =
            memMesh.VertexBuffer.AllocDefault->GetResource()
                ->GetGPUVirtualAddress();
        memMesh.VBV.StrideInBytes = sizeof(float) * 5;
        memMesh.VBV.SizeInBytes   = memMesh.VertexBuffer.AllocCPU.size();

        // Index Buffer
        Acrylic::Util::UploadBuffer(
            memMesh.IndexBuffer.AllocCPU,
            memMesh.IndexBuffer.AllocDefault.GetAddressOf(),
            memMesh.IndexBuffer.AllocUpload.GetAddressOf(),
            CmdList,
            MemAllocator);

        memMesh.IBV.BufferLocation =
            memMesh.IndexBuffer.AllocDefault->GetResource()
                ->GetGPUVirtualAddress();
        memMesh.IBV.Format      = DXGI_FORMAT_R16_UINT;
        memMesh.IBV.SizeInBytes = memMesh.IndexBuffer.AllocCPU.size();
    }

    // Processing TEXTURES (Texture2D)
    for (auto& memMaterial : MemoryMaterials)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC descSRV{
            .Format                  = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D{.MipLevels = 1}};

        if (memMaterial.TexBaseColor.has_value())
        {
            Acrylic::Util::UploadTexture(
                memMaterial.TexBaseColor->AllocCPU,
                memMaterial.TexBaseColor->AllocDefault.GetAddressOf(),
                memMaterial.TexBaseColor->AllocUpload.GetAddressOf(),
                CmdList,
                MemAllocator,
                1024,
                1024);

            // Create SRV for the texture.
            CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
                HeapCSU->GetCPUDescriptorHandleForHeapStart(),
                memMaterial.TexBaseColor->DescriptorHeapOffset,
                StrideCSU};
            Device->CreateShaderResourceView(
                memMaterial.TexBaseColor->AllocDefault->GetResource(),
                &descSRV,
                handleSRV);
        }
        if (memMaterial.TexNormal.has_value())
        {
            Acrylic::Util::UploadTexture(
                memMaterial.TexNormal->AllocCPU,
                memMaterial.TexNormal->AllocDefault.GetAddressOf(),
                memMaterial.TexNormal->AllocUpload.GetAddressOf(),
                CmdList,
                MemAllocator,
                1024,
                1024);

            // Create SRV for the texture.
            CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
                HeapCSU->GetCPUDescriptorHandleForHeapStart(),
                memMaterial.TexNormal->DescriptorHeapOffset,
                StrideCSU};
            Device->CreateShaderResourceView(
                memMaterial.TexNormal->AllocDefault->GetResource(),
                &descSRV,
                handleSRV);
        }
        if (memMaterial.TexARM.has_value())
        {
            Acrylic::Util::UploadTexture(
                memMaterial.TexARM->AllocCPU,
                memMaterial.TexARM->AllocDefault.GetAddressOf(),
                memMaterial.TexARM->AllocUpload.GetAddressOf(),
                CmdList,
                MemAllocator,
                1024,
                1024);

            // Create SRV for the texture.
            CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
                HeapCSU->GetCPUDescriptorHandleForHeapStart(),
                memMaterial.TexARM->DescriptorHeapOffset,
                StrideCSU};
            Device->CreateShaderResourceView(
                memMaterial.TexARM->AllocDefault->GetResource(),
                &descSRV,
                handleSRV);
        }
        if (memMaterial.TexEmissive.has_value())
        {
            Acrylic::Util::UploadTexture(
                memMaterial.TexEmissive->AllocCPU,
                memMaterial.TexEmissive->AllocDefault.GetAddressOf(),
                memMaterial.TexEmissive->AllocUpload.GetAddressOf(),
                CmdList,
                MemAllocator,
                1024,
                1024);

            // Create SRV for the texture.
            CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
                HeapCSU->GetCPUDescriptorHandleForHeapStart(),
                memMaterial.TexEmissive->DescriptorHeapOffset,
                StrideCSU};
            Device->CreateShaderResourceView(
                memMaterial.TexEmissive->AllocDefault->GetResource(),
                &descSRV,
                handleSRV);
        }
    }

    hr = CmdList->Close();
    assert(SUCCEEDED(hr) && "Failed to close command list.");
    std::vector<ID3D12CommandList*> cmdLists{CmdList};
    CmdQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
    Acrylic::Frame::WaitForGPU();
}

void InitECSRegistry()
{
    ECSRegistryInstance = std::make_unique<entt::registry>();

    auto& registry        = *ECSRegistryInstance;
    auto& entities        = ProjectInstance->Entities;
    auto numberOfEntities = entities.size();
    for (auto i = 0; i < numberOfEntities; i++)
    {
        const auto entity = registry.create();
        registry.emplace<ComTag>(entity, entities[i].Tag);

        if (entities[i].Renderable.has_value())
        {
            registry.emplace<ComRenderable>(entity,
                                            entities[i].Renderable.value());
        }
        if (entities[i].Light.has_value())
        {
            registry.emplace<ComLight>(entity, entities[i].Light.value());
        }
        if (entities[i].Camera.has_value())
        {
            registry.emplace<ComCamera>(entity, entities[i].Camera.value());
        }
    }
}

void CreateRS()
{
    std::array<CD3DX12_DESCRIPTOR_RANGE1, 2> ranges{
        CD3DX12_DESCRIPTOR_RANGE1{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                                  1,
                                  0,
                                  0,
                                  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                                  0},
        CD3DX12_DESCRIPTOR_RANGE1{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                                  3,
                                  0,
                                  0,
                                  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                                  1}};

    std::array<CD3DX12_ROOT_PARAMETER1, 2> rootParameters{};
    rootParameters[0].InitAsDescriptorTable(1,
                                            &ranges[0],
                                            D3D12_SHADER_VISIBILITY_ALL);
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

    ComPtr<ID3DBlob> svrs{};
    ComPtr<ID3DBlob> error{};
    hr = D3DX12SerializeVersionedRootSignature(&descRS,
                                               D3D_ROOT_SIGNATURE_VERSION_1_1,
                                               svrs.GetAddressOf(),
                                               error.GetAddressOf());
    assert(SUCCEEDED(hr) && "Failed to serialize versioned root signature.");

    hr = Device->CreateRootSignature(0,
                                     svrs->GetBufferPointer(),
                                     svrs->GetBufferSize(),
                                     IID_PPV_ARGS(RS.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create versioned root signature.");
}

void CreatePSO()
{
    constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 2> descsInputElement{
        D3D12_INPUT_ELEMENT_DESC{"POSITION",
                                 0,
                                 DXGI_FORMAT_R32G32B32_FLOAT,
                                 0,
                                 D3D12_APPEND_ALIGNED_ELEMENT,
                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                 0},
        D3D12_INPUT_ELEMENT_DESC{"TEXCOORD",
                                 0,
                                 DXGI_FORMAT_R32G32_FLOAT,
                                 0,
                                 D3D12_APPEND_ALIGNED_ELEMENT,
                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPSO{};
    descPSO.InputLayout    = D3D12_INPUT_LAYOUT_DESC{descsInputElement.data(),
                                                     descsInputElement.size()};
    descPSO.pRootSignature = RS.Get();
    descPSO.VS             = CD3DX12_SHADER_BYTECODE{
        MemoryMaterials[0].ShaderVertex.AllocCPU.data(),
        MemoryMaterials[0].ShaderVertex.AllocCPU.size()};
    descPSO.PS =
        CD3DX12_SHADER_BYTECODE{MemoryMaterials[0].ShaderPixel.AllocCPU.data(),
                                MemoryMaterials[0].ShaderPixel.AllocCPU.size()};
    descPSO.RasterizerState = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
    descPSO.BlendState      = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
    descPSO.DepthStencilState.DepthEnable   = false;
    descPSO.DepthStencilState.StencilEnable = false;
    descPSO.SampleMask                      = UINT32_MAX;
    descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPSO.NumRenderTargets      = 1;
    descPSO.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
    descPSO.SampleDesc.Count      = 1;

    hr = Device->CreateGraphicsPipelineState(&descPSO,
                                             IID_PPV_ARGS(PSO.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create pipeline state.");
}

void CreateCB()
{
    // CB size must be 256-byte aligned.
    std::uint32_t sizeCB = (sizeof(ConstantBuffer) + 255) & ~255;
    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    CD3DX12_RESOURCE_DESC descRes = CD3DX12_RESOURCE_DESC::Buffer(sizeCB);

    hr = MemAllocator->CreateResource(&descUpload,
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
    hr = AllocCB->GetResource()->Map(0, &readRange, &PointerCB);
    assert(SUCCEEDED(hr) && "Failed to map constant buffer.");
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

    D3D12_DESCRIPTOR_HEAP_DESC heapDescCSU{};
    heapDescCSU.NumDescriptors = 4;
    heapDescCSU.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDescCSU.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = Device->CreateDescriptorHeap(&heapDescCSU,
                                      IID_PPV_ARGS(HeapCSU.GetAddressOf()));
    assert(SUCCEEDED(hr) && "Failed to create CSU descriptor heap.");

    StrideCSU = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    InitScene();
    InitMemoryCPU();
    InitMemoryGPU();
    InitECSRegistry();

    CreateRS();
    CreatePSO();
    CreateCB();

    LOG_INFO("Acrylic::Scene::Init() succeeded.");
}

void Update()
{
    ConstantBuffer cb{};
    XMMATRIX projection{};
    XMMATRIX viewProjection{};

    auto viewLight = ECSRegistryInstance->view<ComLight>();
    viewLight.each([&](const auto& light) -> auto { cb.Color = light.Color; });

    auto viewCamera = ECSRegistryInstance->view<ComCamera>();
    for (const auto& [entity, camera] : viewCamera.each())
    {
        projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(camera.FOV),
                                              camera.AspectRatio,
                                              camera.PlaneNear,
                                              camera.PlaneFar);

        XMVECTOR eye  = XMLoadFloat3(&camera.Position);
        XMVECTOR at   = XMVectorSet(0.0F, 0.0F, 0.0F, 0.0F);
        XMVECTOR up   = XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
        XMMATRIX view = XMMatrixLookAtLH(eye, at, up);

        viewProjection = XMMatrixMultiply(view, projection);
    }

    auto viewRenderable = ECSRegistryInstance->view<ComRenderable>();
    for (const auto& [entity, renderable] : viewRenderable.each())
    {
        // From ComRenderable
        XMFLOAT3 position = renderable.Position; // Translation
        XMFLOAT3 rotation = renderable.Rotation; // Euler angles (radians)
        XMFLOAT3 scale    = renderable.Scale;    // Scale factors

        // Build Model Matrix (Scale → Rotate → Translate)
        XMMATRIX matScale = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX matRotation =
            XMMatrixRotationRollPitchYaw(rotation.x, // Roll (around X)
                                         rotation.y, // Pitch (around Y)
                                         rotation.z  // Yaw (around Z)
            );
        XMMATRIX matTranslation =
            XMMatrixTranslation(position.x, position.y, position.z);

        // Combine: Model = Scale × Rotation × Translation
        XMMATRIX model = matScale * matRotation * matTranslation;

        XMMATRIX mvp = XMMatrixMultiply(model, viewProjection);
        XMStoreFloat4x4(&cb.MVP, XMMatrixTranspose(mvp));
    }
    memcpy(PointerCB, &cb, sizeof(cb));
}

void Render()
{
    auto currentRT  = Acrylic::D3D12::GetCurrentRT();
    auto currentRTV = Acrylic::D3D12::GetCurrentRTV();

    hr = Acrylic::Frame::GetCurrentCA()->Reset();
    assert(SUCCEEDED(hr) && "Failed to reset command allocator.");
    hr = CmdList->Reset(Acrylic::Frame::GetCurrentCA(), PSO.Get());
    assert(SUCCEEDED(hr) && "Failed to reset command list.");

    CD3DX12_VIEWPORT Viewport{0.0F,
                              0.0F,
                              static_cast<float>(Acrylic::Window::GetWidth()),
                              static_cast<float>(Acrylic::Window::GetHeight())};
    CD3DX12_RECT ScissorRect{0,
                             0,
                             static_cast<long>(Acrylic::Window::GetWidth()),
                             static_cast<long>(Acrylic::Window::GetHeight())};

    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->SetGraphicsRootSignature(RS.Get());

    std::vector<ID3D12DescriptorHeap*> heaps{HeapCSU.Get()};
    CmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

    CmdList->SetGraphicsRootDescriptorTable(
        0,
        HeapCSU->GetGPUDescriptorHandleForHeapStart());
    CmdList->SetGraphicsRootDescriptorTable(
        1,
        HeapCSU->GetGPUDescriptorHandleForHeapStart());

    CD3DX12_RESOURCE_BARRIER p2r = CD3DX12_RESOURCE_BARRIER::Transition(
        currentRT,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &p2r);

    CmdList->OMSetRenderTargets(1, &currentRTV, false, nullptr);

    std::vector<float> clearColor{0.0F, 0.2F, 0.4F, 1.0F};
    CmdList->ClearRenderTargetView(currentRTV, clearColor.data(), 0, nullptr);

    auto viewRenderable = ECSRegistryInstance->view<ComRenderable>();
    for (const auto& [entity, renderable] : viewRenderable.each())
    {
        for (const auto& index : renderable.MeshIndices)
        {
            CmdList->IASetVertexBuffers(0, 1, &MemoryMeshes[index].VBV);
            CmdList->IASetIndexBuffer(&MemoryMeshes[index].IBV);
            CmdList->DrawIndexedInstanced(
                MemoryMeshes[index].IndexBuffer.AllocCPU.size() / 2,
                1,
                0,
                0,
                0);
        }
    }

    CD3DX12_RESOURCE_BARRIER r2p =
        CD3DX12_RESOURCE_BARRIER::Transition(currentRT,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET,
                                             D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &r2p);

    hr = CmdList->Close();
    assert(SUCCEEDED(hr) && "Failed to close command list.");

    std::vector<ID3D12CommandList*> cmdLists{CmdList};
    CmdQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());

#ifdef DEBUG
    Acrylic::D3D12::PresentTear();
#else
    Acrylic::D3D12::PresentSync();
#endif
}
//==============================================================================
// Accessors
//==============================================================================

} // namespace Acrylic::Scene