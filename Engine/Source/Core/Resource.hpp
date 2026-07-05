#pragma once

namespace Acrylic::Resource
{
//==============================================================================
// External Struct
//==============================================================================
struct AllocMesh
{
    ComPtr<D3D12MA::Allocation> AllocVB;
    D3D12_VERTEX_BUFFER_VIEW VBV{};

    ComPtr<D3D12MA::Allocation> AllocIB;
    D3D12_INDEX_BUFFER_VIEW IBV{};
};

struct AllocTexture
{
    ComPtr<D3D12MA::Allocation> AllocSR;

    U32 DescriptorIndex{};
    U32 TextureWidth{};
    U32 TextureHeight{};
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
} // namespace Acrylic::Resource
