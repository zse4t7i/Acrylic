#pragma once

#include <ECS.hpp>

namespace Acrylic::Asset
{
//==============================================================================
// External Struct
//==============================================================================
struct ViewFile
{
    Path Path{};
    U32 Offset{0};
    // When Length == 0, it means from Offset to the end of the file.
    U32 Length{0};
};

struct ViewMesh
{
    ViewFile VB{};
    ViewFile IB{};
};

struct ViewMaterial
{
    ViewFile VS{};
    ViewFile PS{};

    optional<ViewFile> TexBaseColor;
    optional<ViewFile> TexNormal;
    optional<ViewFile> TexARM;
    optional<ViewFile> TexEmissive;
};

struct Entity
{
    Acrylic::ECS::ComTag Tag;
    Acrylic::ECS::ComTransform Transformation;

    optional<Acrylic::ECS::ComRenderable> Renderable;
    optional<Acrylic::ECS::ComLight> Light;
    optional<Acrylic::ECS::ComCamera> Camera;
};
//==============================================================================
// External Function
//==============================================================================
void Load(const Path& dirProject);
void Save();

inline auto GetRefEntities() -> vector<Entity>&
{
    static vector<Entity> Entities{};
    return Entities;
}
inline auto GetRefViewMeshes() -> vector<ViewMesh>&
{
    static vector<ViewMesh> ViewMeshes{};
    return ViewMeshes;
}
inline auto GetRefViewMaterials() -> vector<ViewMaterial>&
{
    static vector<ViewMaterial> ViewMaterials{};
    return ViewMaterials;
}
} // namespace Acrylic::Asset
