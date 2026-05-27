#include "Scene.hpp"

using namespace DirectX;

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
struct MemoryAlloc
{
    vector<Byte> AllocCPU;
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
    optional<MemoryAlloc> TexBaseColor;
    optional<MemoryAlloc> TexNormal;
    optional<MemoryAlloc> TexARM;
    optional<MemoryAlloc> TexEmissive;
};

struct DiskView
{
    path Path{};
    U32 Offset{0};
    U32 Length{0};
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
    optional<DiskView> TexBaseColor;
    optional<DiskView> TexNormal;
    optional<DiskView> TexARM;
    optional<DiskView> TexEmissive;
};

struct ComTag
{
    string Name;
};

struct ComRenderable
{
    XMFLOAT3 Position{0.0F, 0.0F, 0.0F};
    XMFLOAT3 Rotation{0.0F, 0.0F, 0.0F};
    XMFLOAT3 Scale{1.0F, 1.0F, 1.0F};
    vector<U32> MeshIndices;
    vector<U32> MaterialIndices;
};

struct ComLight
{
    XMFLOAT3 Position{0.0F, 0.0F, 0.0F};
    XMFLOAT4 Color{1.0F, 1.0F, 1.0F, 1.0F};
    FP32 Intensity{1.0F};
};

struct ComCamera
{
    XMFLOAT3 Position{0.0F, 0.0F, 0.0F};
    FP32 FOV{45.0F};
    FP32 AspectRatio{16.0F / 9.0F};
    FP32 PlaneNear{0.1F};
    FP32 PlaneFar{1000.0F};
};

struct Entity
{
    ComTag Tag;

    optional<ComRenderable> Renderable;
    optional<ComLight> Light;
    optional<ComCamera> Camera;
};

struct Scene
{
    string Name;

    vector<U32> EntityIndices;
};

struct Project
{
    string Name;
    int ActiveSceneIndex{0};

    vector<Scene> Scenes;
    vector<Entity> Entities;
    vector<DiskMesh> DiskMeshes;
    vector<DiskMaterial> DiskMaterials;
};

struct ConstantBuffer
{
    XMFLOAT4X4 MVP{};
    XMFLOAT4 Color{};
};

// Return result for assertion.
HRESULT HR{};
bool BR{};

// External D3D12 Objects
ID3D12Device9 *Device;
ID3D12CommandQueue *CmdQueue;
D3D12MA::Allocator *MemAlctr;

unique_ptr<Project> ProjectInstance{};
unique_ptr<entt::registry> ECSRegistryInstance{};

vector<MemoryMesh> MemoryMeshes{};
vector<MemoryMaterial> MemoryMaterials{};

// Internal D3D12 Objects
ComPtr<ID3D12RootSignature> RS{};
ComPtr<ID3D12PipelineState> PSO{};
ComPtr<D3D12MA::Allocation> AllocCB{};
void *PointerCB{};
ComPtr<ID3D12DescriptorHeap> HeapCSU{};
ComPtr<ID3D12GraphicsCommandList6> CmdList{};
array<ComPtr<ID3D12CommandAllocator>, Acrylic::D3D12::FRAMECOUNT> CmdAlctrs{};

//==============================================================================
// Internal Function
//==============================================================================
void InitInternalD3D12Objects()
{
    { // Create HeapCSU
        D3D12_DESCRIPTOR_HEAP_DESC descHeapCSU{
            .Type{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
            .NumDescriptors{4},
            .Flags{D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE}};

        HR = Device->CreateDescriptorHeap(&descHeapCSU,
                                          IID_PPV_ARGS(HeapCSU.GetAddressOf()));
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
    unordered_map<string, vector<Byte>> loadedFiles{};

    // Pre-loading all resource files from resource folder.
    for (const string &directory : {"Mesh", "Texture", "Shader"})
    {
        if (!std::filesystem::exists(directory))
        {
            continue;
        }

        for (const auto &entry :
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

            const auto &filePath = entry.path();
            // TODO: Move fileContent out of the loop.
            vector<Byte> fileContent{};

            if (filePath.extension() == ".bin")
            {
                BR = Acrylic::Util::LoadBinary(filePath, fileContent);
                assert(BR && "Failed to load binary file.");
            }
            if (filePath.extension() == ".png")
            {
                BR = Acrylic::Util::LoadImage(filePath, fileContent);
                assert(BR && "Failed to load image file.");
            }

            // Use filename as key
            loadedFiles[filePath.filename().string()] = fileContent;
        }
    }

    // Acclocating for meshes.
    for (const auto &diskMesh : ProjectInstance->DiskMeshes)
    {
        assert(diskMesh.VertexBuffer.Path == diskMesh.IndexBuffer.Path &&
               "Vertex and index buffer must be in the same file.");

        const auto &fileContent =
            loadedFiles[diskMesh.VertexBuffer.Path.filename().string()];

        U32 offsetVertex = diskMesh.VertexBuffer.Offset;
        U32 lengthVertex = diskMesh.VertexBuffer.Length == 0
                               ? fileContent.size() - offsetVertex
                               : diskMesh.VertexBuffer.Length;
        U32 offsetIndex = diskMesh.IndexBuffer.Offset;
        U32 lengthIndex = diskMesh.IndexBuffer.Length == 0
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
    for (const auto &diskMaterial : ProjectInstance->DiskMaterials)
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
    auto frameIndex = Acrylic::D3D12::GetFrameIndex();
    auto strideCSU = Acrylic::D3D12::GetStrideCSU();

    HR = CmdAlctrs[frameIndex]->Reset();
    assert(SUCCEEDED(HR) && "Failed to reset command allocator.");
    HR = CmdList->Reset(CmdAlctrs[frameIndex].Get(), PSO.Get());
    assert(SUCCEEDED(HR) && "Failed to reset command list.");

    // Processing BUFFERS (Vertex & Index)
    for (auto &memMesh : MemoryMeshes)
    {
        // Vertex Buffer
        Acrylic::Util::UploadBuffer(
            memMesh.VertexBuffer.AllocCPU,
            memMesh.VertexBuffer.AllocDefault.GetAddressOf(),
            memMesh.VertexBuffer.AllocUpload.GetAddressOf(), CmdList.Get(),
            MemAlctr);

        memMesh.VBV.BufferLocation =
            memMesh.VertexBuffer.AllocDefault->GetResource()
                ->GetGPUVirtualAddress();
        memMesh.VBV.StrideInBytes = sizeof(FP32) * 5;
        memMesh.VBV.SizeInBytes = memMesh.VertexBuffer.AllocCPU.size();

        // Index Buffer
        Acrylic::Util::UploadBuffer(
            memMesh.IndexBuffer.AllocCPU,
            memMesh.IndexBuffer.AllocDefault.GetAddressOf(),
            memMesh.IndexBuffer.AllocUpload.GetAddressOf(), CmdList.Get(),
            MemAlctr);

        memMesh.IBV.BufferLocation =
            memMesh.IndexBuffer.AllocDefault->GetResource()
                ->GetGPUVirtualAddress();
        memMesh.IBV.Format = DXGI_FORMAT_R16_UINT;
        memMesh.IBV.SizeInBytes = memMesh.IndexBuffer.AllocCPU.size();
    }

    // Processing TEXTURES (Texture2D)
    for (auto &memMaterial : MemoryMaterials)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC descSRV{
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D{.MipLevels = 1}};

        if (memMaterial.TexBaseColor.has_value())
        {
            Acrylic::Util::UploadTexture(
                memMaterial.TexBaseColor->AllocCPU,
                memMaterial.TexBaseColor->AllocDefault.GetAddressOf(),
                memMaterial.TexBaseColor->AllocUpload.GetAddressOf(),
                CmdList.Get(), MemAlctr, 1024, 1024);

            // Create SRV for the texture.
            CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
                HeapCSU->GetCPUDescriptorHandleForHeapStart(),
                memMaterial.TexBaseColor->DescriptorHeapOffset, strideCSU};
            Device->CreateShaderResourceView(
                memMaterial.TexBaseColor->AllocDefault->GetResource(), &descSRV,
                handleSRV);
        }
        if (memMaterial.TexNormal.has_value())
        {
            Acrylic::Util::UploadTexture(
                memMaterial.TexNormal->AllocCPU,
                memMaterial.TexNormal->AllocDefault.GetAddressOf(),
                memMaterial.TexNormal->AllocUpload.GetAddressOf(),
                CmdList.Get(), MemAlctr, 1024, 1024);

            // Create SRV for the texture.
            CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
                HeapCSU->GetCPUDescriptorHandleForHeapStart(),
                memMaterial.TexNormal->DescriptorHeapOffset, strideCSU};
            Device->CreateShaderResourceView(
                memMaterial.TexNormal->AllocDefault->GetResource(), &descSRV,
                handleSRV);
        }
        if (memMaterial.TexARM.has_value())
        {
            Acrylic::Util::UploadTexture(
                memMaterial.TexARM->AllocCPU,
                memMaterial.TexARM->AllocDefault.GetAddressOf(),
                memMaterial.TexARM->AllocUpload.GetAddressOf(), CmdList.Get(),
                MemAlctr, 1024, 1024);

            // Create SRV for the texture.
            CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
                HeapCSU->GetCPUDescriptorHandleForHeapStart(),
                memMaterial.TexARM->DescriptorHeapOffset, strideCSU};
            Device->CreateShaderResourceView(
                memMaterial.TexARM->AllocDefault->GetResource(), &descSRV,
                handleSRV);
        }
        if (memMaterial.TexEmissive.has_value())
        {
            Acrylic::Util::UploadTexture(
                memMaterial.TexEmissive->AllocCPU,
                memMaterial.TexEmissive->AllocDefault.GetAddressOf(),
                memMaterial.TexEmissive->AllocUpload.GetAddressOf(),
                CmdList.Get(), MemAlctr, 1024, 1024);

            // Create SRV for the texture.
            CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV{
                HeapCSU->GetCPUDescriptorHandleForHeapStart(),
                memMaterial.TexEmissive->DescriptorHeapOffset, strideCSU};
            Device->CreateShaderResourceView(
                memMaterial.TexEmissive->AllocDefault->GetResource(), &descSRV,
                handleSRV);
        }
    }

    HR = CmdList->Close();
    assert(SUCCEEDED(HR) && "Failed to close command list.");

    vector<ID3D12CommandList *> cmdLists{CmdList.Get()};
    CmdQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
    Acrylic::D3D12::WaitForCmdExecuted();
}

void InitECSRegistry()
{
    ECSRegistryInstance = std::make_unique<entt::registry>();

    auto &registry = *ECSRegistryInstance;
    auto &entities = ProjectInstance->Entities;
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
    array<CD3DX12_DESCRIPTOR_RANGE1, 2> ranges{
        CD3DX12_DESCRIPTOR_RANGE1{D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0,
                                  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC, 0},
        CD3DX12_DESCRIPTOR_RANGE1{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0,
                                  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC, 1}};

    array<CD3DX12_ROOT_PARAMETER1, 2> rootParameters{};
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0],
                                            D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[1],
                                            D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0F;
    sampler.MaxLOD = FP32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Allow input layout and deny uneccessary access to certain pipeline
    // stages.
    D3D12_ROOT_SIGNATURE_FLAGS flagRS =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC descRS{};
    descRS.Init_1_1(rootParameters.size(), rootParameters.data(), 1, &sampler,
                    flagRS);

    ComPtr<ID3DBlob> svrs{};
    ComPtr<ID3DBlob> error{};
    HR = D3DX12SerializeVersionedRootSignature(
        &descRS, D3D_ROOT_SIGNATURE_VERSION_1_1, svrs.GetAddressOf(),
        error.GetAddressOf());
    assert(SUCCEEDED(HR) && "Failed to serialize versioned root signature.");

    HR = Device->CreateRootSignature(0, svrs->GetBufferPointer(),
                                     svrs->GetBufferSize(),
                                     IID_PPV_ARGS(RS.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create versioned root signature.");
}

void CreatePSO()
{
    constexpr array<D3D12_INPUT_ELEMENT_DESC, 2> descsInputElement{
        D3D12_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                                 D3D12_APPEND_ALIGNED_ELEMENT,
                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        D3D12_INPUT_ELEMENT_DESC{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                                 D3D12_APPEND_ALIGNED_ELEMENT,
                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPSO{};
    descPSO.InputLayout = D3D12_INPUT_LAYOUT_DESC{descsInputElement.data(),
                                                  descsInputElement.size()};
    descPSO.pRootSignature = RS.Get();
    descPSO.VS = CD3DX12_SHADER_BYTECODE{
        MemoryMaterials[0].ShaderVertex.AllocCPU.data(),
        MemoryMaterials[0].ShaderVertex.AllocCPU.size()};
    descPSO.PS =
        CD3DX12_SHADER_BYTECODE{MemoryMaterials[0].ShaderPixel.AllocCPU.data(),
                                MemoryMaterials[0].ShaderPixel.AllocCPU.size()};
    descPSO.RasterizerState = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
    descPSO.BlendState = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
    descPSO.DepthStencilState.DepthEnable = false;
    descPSO.DepthStencilState.StencilEnable = false;
    descPSO.SampleMask = U32_MAX;
    descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPSO.NumRenderTargets = 1;
    descPSO.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    descPSO.SampleDesc.Count = 1;

    HR = Device->CreateGraphicsPipelineState(&descPSO,
                                             IID_PPV_ARGS(PSO.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create pipeline state.");
}

void CreateCB()
{
    // CB size must be 256-byte aligned.
    U32 sizeCB = (sizeof(ConstantBuffer) + 255) & ~255;
    D3D12MA::CALLOCATION_DESC descUpload{
        D3D12_HEAP_TYPE_UPLOAD, D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY};
    CD3DX12_RESOURCE_DESC descRes = CD3DX12_RESOURCE_DESC::Buffer(sizeCB);

    HR = MemAlctr->CreateResource(&descUpload, &descRes,
                                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                  AllocCB.GetAddressOf(), IID_NULL, nullptr);
    assert(SUCCEEDED(HR) && "Failed to create constant buffer.");

    D3D12_CONSTANT_BUFFER_VIEW_DESC descCBV{};
    descCBV.BufferLocation = AllocCB->GetResource()->GetGPUVirtualAddress();
    descCBV.SizeInBytes = sizeCB;
    Device->CreateConstantBufferView(
        &descCBV, HeapCSU->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RANGE readRange{0, 0};
    HR = AllocCB->GetResource()->Map(0, &readRange, &PointerCB);
    assert(SUCCEEDED(HR) && "Failed to map constant buffer.");
}
} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Scene
{
//==============================================================================
// External Function
//==============================================================================
void Init()
{
    Device = Acrylic::D3D12::GetDevice();
    MemAlctr = Acrylic::D3D12::GetMemAllocator();
    CmdQueue = Acrylic::D3D12::GetCmdQueue();

    InitInternalD3D12Objects();

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
    viewLight.each([&](const auto &light) -> auto { cb.Color = light.Color; });

    auto viewCamera = ECSRegistryInstance->view<ComCamera>();
    for (const auto &[entity, camera] : viewCamera.each())
    {
        projection = XMMatrixPerspectiveFovLH(
            XMConvertToRadians(camera.FOV), camera.AspectRatio,
            camera.PlaneNear, camera.PlaneFar);

        XMVECTOR eye = XMLoadFloat3(&camera.Position);
        XMVECTOR at = XMVectorSet(0.0F, 0.0F, 0.0F, 0.0F);
        XMVECTOR up = XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
        XMMATRIX view = XMMatrixLookAtLH(eye, at, up);

        viewProjection = XMMatrixMultiply(view, projection);
    }

    auto viewRenderable = ECSRegistryInstance->view<ComRenderable>();
    for (const auto &[entity, renderable] : viewRenderable.each())
    {
        // From ComRenderable
        XMFLOAT3 position = renderable.Position; // Translation
        XMFLOAT3 rotation = renderable.Rotation; // Euler angles (radians)
        XMFLOAT3 scale = renderable.Scale;       // Scale factors

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
    auto *currentRT = Acrylic::D3D12::GetCurrentRT();
    auto currentRTV = Acrylic::D3D12::GetCurrentRTV();
    auto frameIndex = Acrylic::D3D12::GetFrameIndex();

    HR = CmdAlctrs[frameIndex]->Reset();
    assert(SUCCEEDED(HR) && "Failed to reset command allocator.");
    HR = CmdList->Reset(CmdAlctrs[frameIndex].Get(), PSO.Get());
    assert(SUCCEEDED(HR) && "Failed to reset command list.");

    CD3DX12_VIEWPORT Viewport{0.0F, 0.0F,
                              static_cast<FP32>(Acrylic::Window::GetWidth()),
                              static_cast<FP32>(Acrylic::Window::GetHeight())};

    CD3DX12_RECT ScissorRect{0, 0, Acrylic::Window::GetWidth(),
                             Acrylic::Window::GetHeight()};

    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->SetGraphicsRootSignature(RS.Get());

    vector<ID3D12DescriptorHeap *> heaps{HeapCSU.Get()};
    CmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

    CmdList->SetGraphicsRootDescriptorTable(
        0, HeapCSU->GetGPUDescriptorHandleForHeapStart());
    CmdList->SetGraphicsRootDescriptorTable(
        1, HeapCSU->GetGPUDescriptorHandleForHeapStart());

    auto p2r = CD3DX12_RESOURCE_BARRIER::Transition(
        currentRT, D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &p2r);

    CmdList->OMSetRenderTargets(1, &currentRTV, false, nullptr);

    vector<FP32> clearColor{0.0F, 0.2F, 0.4F, 1.0F};
    CmdList->ClearRenderTargetView(currentRTV, clearColor.data(), 0, nullptr);

    auto viewRenderable = ECSRegistryInstance->view<ComRenderable>();
    for (const auto &[entity, renderable] : viewRenderable.each())
    {
        for (const auto &index : renderable.MeshIndices)
        {
            CmdList->IASetVertexBuffers(0, 1, &MemoryMeshes[index].VBV);
            CmdList->IASetIndexBuffer(&MemoryMeshes[index].IBV);
            CmdList->DrawIndexedInstanced(
                MemoryMeshes[index].IndexBuffer.AllocCPU.size() / 2, 1, 0, 0,
                0);
        }
    }

    auto r2p = CD3DX12_RESOURCE_BARRIER::Transition(
        currentRT, D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &r2p);

    HR = CmdList->Close();
    assert(SUCCEEDED(HR) && "Failed to close command list.");

    vector<ID3D12CommandList *> cmdLists{CmdList.Get()};
    CmdQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
}
//==============================================================================
// Accessors
//==============================================================================

} // namespace Acrylic::Scene
#pragma endregion