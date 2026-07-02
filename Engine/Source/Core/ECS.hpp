#pragma once

namespace Acrylic::ECS
{
//==============================================================================
// External Struct
//==============================================================================
struct ComTag
{
    string Name;
};

struct ComTransform
{
    XMFLOAT3 Translation{0.0F, 0.0F, 0.0F};
    XMFLOAT3 Rotation{0.0F, 0.0F, 0.0F};
    XMFLOAT3 Scaling{1.0F, 1.0F, 1.0F};
};

struct ComRenderable
{
    vector<U32> MeshIndices;
    vector<U32> MaterialIndices;

    // Primitive topology.
    D3D_PRIMITIVE_TOPOLOGY Topology{D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST};

    // DrawIndexedInstanced parameters.
    UINT IndexCount{0};
    UINT StartIndexLocation{0};
    I32 BaseVertexLocation{0};
};

struct ComLight
{
    XMFLOAT4 Color{1.0F, 1.0F, 1.0F, 1.0F};
    F32 Intensity{1.0F};
};

struct ComCamera
{
    XMFLOAT3 LookAt{0.0F, 0.0F, 0.0F};
    F32 FOV{45.0F};
    F32 AspectRatio{16.0F / 9.0F};
    F32 PlaneNear{0.1F};
    F32 PlaneFar{100.0F};
};
//==============================================================================
// External Function
//==============================================================================
void Init();

inline auto GetRegistry() -> entt::registry&
{
    static entt::registry Registry{};
    return Registry;
}
} // namespace Acrylic::ECS
