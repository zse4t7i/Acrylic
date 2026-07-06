#include "Asset.hpp"

using namespace Acrylic::Asset;

#pragma region Internal
namespace
{
//==============================================================================
// Internal Variable
//==============================================================================
// Return result for assertion.
HRESULT HR{};
bool BR{};

Path DirProject{};

//==============================================================================
// Internal Function
//==============================================================================
void LoadDefaultAsset()
{
    auto& entities      = Acrylic::Asset::GetRefEntities();
    auto& viewMeshes    = Acrylic::Asset::GetRefViewMeshes();
    auto& viewMaterials = Acrylic::Asset::GetRefViewMaterials();

    entities.emplace_back(
        Entity{.Tag{.Name{"Cube"}},
               .Transformation{},
               .Renderable{Acrylic::ECS::ComRenderable{.MeshIndices{0},
                                                       .MaterialIndices{0}}}});
    entities.emplace_back(
        Entity{.Tag{.Name{"Light"}},
               .Transformation{.Translation{4.0F, 4.0F, 4.0F}},
               .Light{{}}});
    entities.emplace_back(
        Entity{.Tag{.Name{"Camera"}},
               .Transformation{.Translation{2.0F, 2.0F, 2.0F}},
               .Camera{{}}});

    viewMeshes.emplace_back(
        ViewMesh{.VB{.Path{"Mesh/Cube.bin"}, .Offset{0}, .Length{480}},
                 .IB{
                     .Path{"Mesh/Cube.bin"},
                     .Offset{480},
                     .Length{72},
                 }});
    viewMaterials.emplace_back(ViewMaterial{
        .VS{.Path{"Shader/Cube.vs.bin"}, .Offset{0}, .Length{0}},
        .PS{.Path{"Shader/Cube.ps.bin"}, .Offset{0}, .Length{0}},
        .TexBaseColor{
            {.Path{"Texture/Metal_BaseColor.png"}, .Offset{0}, .Length{0}}},
        .TexNormal{{.Path{"Texture/Metal_Normal.png"}, .Offset{0}, .Length{0}}},
        .TexARM{{.Path{"Texture/Metal_ARM.png"}, .Offset{0}, .Length{0}}}});
}
} // namespace
#pragma endregion

#pragma region External
namespace Acrylic::Asset
{
//==============================================================================
// External Function
//==============================================================================
void Load(const Path& dirProject)
{
    DirProject = dirProject;

    // TODO: Temporary implementation for testing.
    LoadDefaultAsset();

    LOG_INFO("Acrylic::Asset::Init() succeeded.");
}
} // namespace Acrylic::Asset
#pragma endregion