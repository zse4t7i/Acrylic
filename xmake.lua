set_project("Acrylic")
set_version("0.0.1", {build = "%Y%m%d%H%M"})
set_xmakever("3.0.8")
set_languages("c++20")
set_toolchains("msvc")
set_encodings("utf-8")
set_policy("build.progress_style", "multirow")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

if is_mode("debug") then
    add_defines("DEBUG", "_DEBUG")
elseif is_mode("release") then
    add_defines("NDEBUG")
    add_defines("RELEASE")
    add_defines("QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO")
end

add_defines("QUILL_DISABLE_NON_PREFIXED_MACROS")
add_defines("D3D12MA_USING_DIRECTX_HEADERS")

add_requires("quill")
add_requires("benchmark")
add_requires("nlohmann_json")
add_requires("stb")
add_requires("tbb")
add_requires("entt")
add_requires("tinygltf")
add_requires("imgui[dx12,win32,freetype] v1.92.7-docking")
add_requires("d3d12-memory-allocator")
add_requires("directxtex")
-- add_requires("directxmesh")
add_requires("directxmath")
add_requires("directx-headers")
add_requires("directx12-agility")

includes("Engine", "Project")
