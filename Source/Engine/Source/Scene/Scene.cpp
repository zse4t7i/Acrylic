#include "Scene.hpp"
#include "Asset.hpp"
#include "ECS.hpp"
#include "Resource.hpp"

using namespace DirectX;
using namespace Acrylic::ECS;

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================

struct ConstantObject
{
    XMFLOAT4X4 MatrixW{};
};

struct ConstantFrame
{
    XMFLOAT4X4 MatrixVP{};
    XMFLOAT4 LightColor{};
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
ID3D12Device9* Device;
ID3D12CommandQueue* CmdQueue;
D3D12MA::Allocator* AlctrGPU;

// Internal D3D12 Objects
ComPtr<ID3D12RootSignature> RS{};
ComPtr<ID3D12PipelineState> PSO{};
Acrylic::AllocationDynamic AllocCB{};
ComPtr<ID3D12GraphicsCommandList6> CmdList{};
array<ComPtr<ID3D12CommandAllocator>, Acrylic::D3D12::FRAMECOUNT> CmdAlctrs{};
array<Acrylic::AllocatorDynamic, Acrylic::D3D12::FRAMECOUNT> AlctrDynamics{};

Acrylic::DescriptorPoolCSU PoolSRV{};
//==============================================================================
// Internal Function
//==============================================================================
void InitInternalD3D12Objects()
{
    HR = Device->CreateCommandList1(0,
                                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    D3D12_COMMAND_LIST_FLAG_NONE,
                                    IID_PPV_ARGS(CmdList.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create command list.");

    for (int i = 0; i < Acrylic::D3D12::FRAMECOUNT; i++)
    {
        HR = Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CmdAlctrs[i].GetAddressOf()));
        assert(SUCCEEDED(HR) && "Failed to create command allocator.");
    }
}

void InitECSRegistry()
{
    auto& registry = Acrylic::ECS::GetRefRegistry();
    auto& entities = Acrylic::Asset::GetRefEntities();

    auto numberOfEntities = entities.size();
    for (int i = 0; i < numberOfEntities; i++)
    {
        const auto entity = registry.create();
        registry.emplace<ComTag>(entity, entities[i].Tag);
        registry.emplace<ComTransform>(entity, entities[i].Transformation);

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
    auto range =
        CD3DX12_DESCRIPTOR_RANGE1{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                                  3,
                                  0,
                                  0,
                                  D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                                  0};

    array<CD3DX12_ROOT_PARAMETER1, 2> rootParameters{};
    rootParameters[0].InitAsConstantBufferView(0);
    rootParameters[1].InitAsDescriptorTable(1,
                                            &range,
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
    sampler.MaxLOD           = F32_MAX;
    sampler.ShaderRegister   = 0;
    sampler.RegisterSpace    = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Allow input layout and deny uneccessary access to certain pipeline
    // stages.
    D3D12_ROOT_SIGNATURE_FLAGS flagRS =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC descRS{};
    descRS.Init_1_1(rootParameters.size(),
                    rootParameters.data(),
                    1,
                    &sampler,
                    flagRS);

    ComPtr<ID3DBlob> svrs{};
    ComPtr<ID3DBlob> error{};
    HR = D3DX12SerializeVersionedRootSignature(&descRS,
                                               D3D_ROOT_SIGNATURE_VERSION_1_1,
                                               svrs.GetAddressOf(),
                                               error.GetAddressOf());
    assert(SUCCEEDED(HR) && "Failed to serialize versioned root signature.");

    HR = Device->CreateRootSignature(0,
                                     svrs->GetBufferPointer(),
                                     svrs->GetBufferSize(),
                                     IID_PPV_ARGS(RS.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create versioned root signature.");
}

void CreatePSO()
{
    auto& viewMaterials = Acrylic::Asset::GetRefViewMaterials();
    vector<Byte> viewVS{};
    vector<Byte> viewPS{};

    BR = Acrylic::Util::LoadBinary(viewMaterials[0].VS.Path, viewVS);
    assert(BR && "Failed to load binary file.");
    BR = Acrylic::Util::LoadBinary(viewMaterials[0].PS.Path, viewPS);
    assert(BR && "Failed to load binary file.");

    constexpr array<D3D12_INPUT_ELEMENT_DESC, 2> descsInputElement{
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
    descPSO.VS = CD3DX12_SHADER_BYTECODE{viewVS.data(), viewVS.size()};
    descPSO.PS = CD3DX12_SHADER_BYTECODE{viewPS.data(), viewPS.size()};
    descPSO.RasterizerState = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};
    descPSO.BlendState      = CD3DX12_BLEND_DESC{D3D12_DEFAULT};
    descPSO.DepthStencilState.DepthEnable   = false;
    descPSO.DepthStencilState.StencilEnable = false;
    descPSO.SampleMask                      = U32_MAX;
    descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPSO.NumRenderTargets      = 1;
    descPSO.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
    descPSO.SampleDesc.Count      = 1;

    HR = Device->CreateGraphicsPipelineState(&descPSO,
                                             IID_PPV_ARGS(PSO.GetAddressOf()));
    assert(SUCCEEDED(HR) && "Failed to create pipeline state.");
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
    Device   = Acrylic::D3D12::GetPtrDevice();
    AlctrGPU = Acrylic::D3D12::GetPtrAlctrGPU();
    CmdQueue = Acrylic::D3D12::GetPtrCmdQueue();

    PoolSRV.Init(Device, 1024);

    Acrylic::Resource::Init();
    Acrylic::Resource::BeginAllocate();
    Acrylic::Resource::AllocateAll(PoolSRV);
    auto future = Acrylic::Resource::EndAllocate();

    for (int i = 0; i < Acrylic::D3D12::FRAMECOUNT; i++)
    {
        AlctrDynamics[i].Init(AlctrGPU, 64 * 1024);
    }

    InitInternalD3D12Objects();
    InitECSRegistry();

    CreateRS();
    CreatePSO();

    future.get();
    LOG_INFO("Acrylic::Scene::Init() succeeded.");
}

void Update()
{
    auto alctrDynamic = AlctrDynamics[Acrylic::D3D12::GetFrameIndex()];
    alctrDynamic.Reset();
    AllocCB = alctrDynamic.Allocate(sizeof(ConstantBuffer), 256);

    ConstantBuffer cb{};
    XMMATRIX projection{};
    XMMATRIX viewProjection{};

    auto& inputState = Acrylic::Input::GetState();
    auto& registry   = Acrylic::ECS::GetRefRegistry();

    auto viewLight = registry.view<ComLight>();
    viewLight.each([&](const auto& light) -> auto { cb.Color = light.Color; });

    auto viewCamera = registry.view<ComTransform, ComCamera>();
    for (const auto& [entity, transformation, camera] : viewCamera.each())
    {
        projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(camera.FOV),
                                              camera.AspectRatio,
                                              camera.PlaneNear,
                                              camera.PlaneFar);

        // TODO: NEED REWRITE! Simple camera control for test.
        // FPS camera control using input system
        XMVECTOR eye = XMLoadFloat3(&transformation.Translation);
        XMVECTOR at  = XMLoadFloat3(&camera.LookAt);
        XMVECTOR up  = XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);

        // Calculate forward, right, and up vectors for camera movement
        XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(at, eye));
        XMVECTOR right   = XMVector3Normalize(XMVector3Cross(forward, up));
        XMVECTOR trueUp  = XMVector3Cross(right, forward);

        // Movement speed
        F32 moveSpeed    = 0.001F; // Base movement speed per frame
        F32 acceleration = 2.0F;   // Speed multiplier when holding LSHIFT

        // Check if LSHIFT is held (index 8)
        const bool isShiftHeld =
            inputState.KeyboardKeys[8] == Acrylic::Input::ButtonState::Held;
        F32 currentSpeed = isShiftHeld ? moveSpeed * acceleration : moveSpeed;

        // Handle forward/backward movement (W/S or UP/DOWN)
        if (inputState.KeyboardKeys[0] == Acrylic::Input::ButtonState::Held ||
            inputState.KeyboardKeys[4] ==
                Acrylic::Input::ButtonState::Held) // W
            eye = XMVectorAdd(eye, XMVectorScale(forward, currentSpeed));
        if (inputState.KeyboardKeys[1] == Acrylic::Input::ButtonState::Held ||
            inputState.KeyboardKeys[5] ==
                Acrylic::Input::ButtonState::Held) // S
            eye = XMVectorSubtract(eye, XMVectorScale(forward, currentSpeed));

        // Handle left/right movement (A/D or LEFT/RIGHT)
        if (inputState.KeyboardKeys[2] == Acrylic::Input::ButtonState::Held ||
            inputState.KeyboardKeys[6] ==
                Acrylic::Input::ButtonState::Held) // A
            eye = XMVectorAdd(eye, XMVectorScale(right, currentSpeed));
        if (inputState.KeyboardKeys[3] == Acrylic::Input::ButtonState::Held ||
            inputState.KeyboardKeys[7] ==
                Acrylic::Input::ButtonState::Held) // D
            eye = XMVectorSubtract(eye, XMVectorScale(right, currentSpeed));

        // Update camera position
        XMStoreFloat3(&transformation.Translation, eye);

        // Handle mouse look for camera direction
        F32 mouseSensitivity = 0.001F;
        XMVECTOR yawAxis     = trueUp; // Rotate around world up for yaw
        XMVECTOR pitchAxis   = right;  // Rotate around right vector for pitch

        // Apply yaw (horizontal mouse movement - DeltaPX)
        if (inputState.DeltaPX != 0.0F)
        {
            XMMATRIX yawRotation =
                XMMatrixRotationAxis(yawAxis,
                                     inputState.DeltaPX * mouseSensitivity);
            forward = XMVector3TransformCoord(forward, yawRotation);
        }

        // Apply pitch (vertical mouse movement - DeltaPY)
        if (inputState.DeltaPY != 0.0F)
        {
            XMMATRIX pitchRotation =
                XMMatrixRotationAxis(pitchAxis,
                                     inputState.DeltaPY * mouseSensitivity);
            forward = XMVector3TransformCoord(forward, pitchRotation);
        }

        // Update camera direction
        at = XMVectorAdd(eye, forward);
        XMStoreFloat3(&camera.LookAt, at);

        // Calculate view and projection matrices
        XMMATRIX view  = XMMatrixLookAtLH(eye, at, trueUp);
        viewProjection = XMMatrixMultiply(view, projection);

        // XMVECTOR eye  = XMLoadFloat3(&camera.Position);
        // XMVECTOR at   = XMLoadFloat3(&camera.Direction);
        // XMVECTOR up   = XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
        // XMMATRIX view = XMMatrixLookAtLH(eye, at, up);
        // viewProjection = XMMatrixMultiply(view, projection);
    }

    auto viewRenderable = registry.view<ComTransform, ComRenderable>();
    for (const auto& [entity, transformation, renderable] :
         viewRenderable.each())
    {
        // From ComRenderable
        XMFLOAT3 position = transformation.Translation; // Translation
        XMFLOAT3 rotation = transformation.Rotation; // Euler angles (radians)
        XMFLOAT3 scale    = transformation.Scaling;  // Scale factors

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
    memcpy(AllocCB.AddressCPU, &cb, sizeof(cb));
}

void Render()
{
    auto& allocMeshes    = Acrylic::Resource::GetRefAllocMeshes();
    auto& allocMaterials = Acrylic::Resource::GetRefAllocMaterials();
    auto& registry       = Acrylic::ECS::GetRefRegistry();
    auto* currentRT      = Acrylic::D3D12::GetPtrCurrentRT();
    auto currentRTV      = Acrylic::D3D12::GetCurrentRTV();
    auto frameIndex      = Acrylic::D3D12::GetFrameIndex();

    HR = CmdAlctrs[frameIndex]->Reset();
    assert(SUCCEEDED(HR) && "Failed to reset command allocator.");
    HR = CmdList->Reset(CmdAlctrs[frameIndex].Get(), PSO.Get());
    assert(SUCCEEDED(HR) && "Failed to reset command list.");

    CD3DX12_VIEWPORT Viewport{0.0F,
                              0.0F,
                              static_cast<F32>(Acrylic::Window::GetWidth()),
                              static_cast<F32>(Acrylic::Window::GetHeight())};

    CD3DX12_RECT ScissorRect{0,
                             0,
                             Acrylic::Window::GetWidth(),
                             Acrylic::Window::GetHeight()};

    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->SetGraphicsRootSignature(RS.Get());

    vector<ID3D12DescriptorHeap*> heaps{PoolSRV.GetPtrHeap()};
    CmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

    CmdList->SetGraphicsRootConstantBufferView(0, AllocCB.AddressGPU);
    CmdList->SetGraphicsRootDescriptorTable(
        1,
        PoolSRV.GetPtrHeap()->GetGPUDescriptorHandleForHeapStart());

    auto p2r = CD3DX12_RESOURCE_BARRIER::Transition(
        currentRT,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &p2r);

    CmdList->OMSetRenderTargets(1, &currentRTV, false, nullptr);

    vector<F32> clearColor{0.0F, 0.2F, 0.4F, 1.0F};
    CmdList->ClearRenderTargetView(currentRTV, clearColor.data(), 0, nullptr);

    auto viewRenderable = registry.view<ComRenderable>();
    for (const auto& [entity, renderable] : viewRenderable.each())
    {
        for (const auto& index : renderable.MeshIndices)
        {
            CmdList->IASetVertexBuffers(0, 1, &allocMeshes[index].VBV);
            CmdList->IASetIndexBuffer(&allocMeshes[index].IBV);
            CmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
        }
    }

    auto r2p =
        CD3DX12_RESOURCE_BARRIER::Transition(currentRT,
                                             D3D12_RESOURCE_STATE_RENDER_TARGET,
                                             D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &r2p);

    HR = CmdList->Close();
    assert(SUCCEEDED(HR) && "Failed to close command list.");

    vector<ID3D12CommandList*> cmdLists{CmdList.Get()};
    CmdQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
}
//==============================================================================
// Accessors
//==============================================================================

} // namespace Acrylic::Scene
#pragma endregion