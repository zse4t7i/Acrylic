#pragma once

namespace Acrylic::Engine::Resource
{
//==============================================================================
// External Struct
//==============================================================================
struct AllocBuffer
{
    ComPtr<D3D12MA::Allocation> AllocGPU;
};

struct AllocMesh
{
    AllocBuffer VB;
    D3D12_VERTEX_BUFFER_VIEW VBV{};

    AllocBuffer IB;
    D3D12_INDEX_BUFFER_VIEW IBV{};
};

struct AllocTexture
{
    ComPtr<D3D12MA::Allocation> AllocGPU;

    U32 DescriptorIndex{};
};

struct AllocMaterial
{
    optional<AllocTexture> TexBaseColor;
    optional<AllocTexture> TexNormal;
    optional<AllocTexture> TexARM;
    optional<AllocTexture> TexEmissive;
};

//==============================================================================
// External Function
//==============================================================================
void Init();
void BeginAllocate();
void AllocateAll(Acrylic::Engine::DescriptorPoolCSU& poolSRV);
auto EndAllocate() -> future<void>;

inline auto GetRefAllocMeshes() -> vector<AllocMesh>&
{
    static vector<AllocMesh> AllocMeshes{};
    return AllocMeshes;
}
inline auto GetRefAllocMaterials() -> vector<AllocMaterial>&
{
    static vector<AllocMaterial> AllocMaterials{};
    return AllocMaterials;
}
} // namespace Acrylic::Engine::Resource
