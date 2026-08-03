set_project("Acrylic")
set_version("0.0.1", {build = "%Y%m%d%H%M"})
set_xmakever("3.0.8")
set_languages("c++20")
set_toolchains("msvc")
set_encodings("utf-8")
set_fpmodels("fast")
set_policy("build.progress_style", "multirow")

add_vectorexts("all")
add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

if is_mode("debug") then
    add_defines("DEBUG", "_DEBUG")
elseif is_mode("release") then
    add_defines("NDEBUG")
    add_defines("RELEASE")
end

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
add_requires("nuget::Microsoft.GameInput", {alias = "GameInput"})
add_requires("nuget::WinPixEventRuntime", {alias = "PIXEventRuntime"})

includes("Xmake/**.lua")
includes("Source/Engine", "Source/Editor", "Source/Launcher", "Source/Project")