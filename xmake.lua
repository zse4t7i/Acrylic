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

add_requires(
    "quill",
    "benchmark",
    "nlohmann_json",
    "stb",
    "tbb",
    "entt",
    "tinygltf",
    "imgui[dx12,win32,freetype] v1.92.7-docking",
    "d3d12-memory-allocator",
    "directxtex",
    -- "directxmesh",
    "directxmath",
    "directx-headers",
    "directx12-agility",
    "nuget::Microsoft.GameInput", {alias = "GameInput"},
    "nuget::WinPixEventRuntime", {alias = "PIXEventRuntime"})

-- includes("Xmake/**.lua")
-- includes("Source/Engine", "Source/Editor", "Source/Launcher", "Source/Project")

rule("CopyAcrylicAsset")
    before_build(function (target)
        os.cp("Asset/Font/", target:targetdir(), {copy_if_different = true})

        cprint("${bright green}Engine's resources copied!")
    end)


rule("CopyD3D12AgilitySDK")
    before_build(function (target)
        -- print(string.format([[srcDir is %s]], target:pkg("directx12-agility"):installdir()))
        local srcDir = target:pkg("directx12-agility"):installdir()
        local dstDir = path.join(target:targetdir(), "D3D12")

        -- Copy AgilitySDK DLLs to the D3D12/ directory.
        os.cp(path.join(srcDir, "bin", "D3D12Core.dll"), path.join(dstDir, "D3D12Core.dll"))
        if is_mode("debug") then
            os.cp(path.join(srcDir, "bin", "d3d12SDKLayers.dll"), path.join(dstDir, "d3d12SDKLayers.dll"))
        end

        cprint("${bright green}D3D12 Agility SDK copied!")

        -- Parse the minor version number and define D3D12_AGILITY_SDK_VERSION.
        local version = target:pkg("directx12-agility"):version():minor()
        target:add("defines", "D3D12_AGILITY_SDK_VERSION=" .. version)
    end)

target("AcrylicEngine", function ()
    set_kind("static")
    set_pcxxheader("Source/Engine/PCH.hpp")

    add_rules("CopyAcrylicAsset")
    
    add_syslinks("user32", "d3d12", "dxgi")

if is_mode("release") then
    add_defines("QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO")
end
    add_defines("QUILL_DISABLE_NON_PREFIXED_MACROS")
    add_defines("D3D12MA_USING_DIRECTX_HEADERS")

    add_files("Source/Engine/**.cpp")

    add_includedirs("Source/Engine/", {public=true})
    add_includedirs("Source/Engine/Asset", {public=true})
    add_includedirs("Source/Engine/Core", {public=true})
    add_includedirs("Source/Engine/Graphics", {public=true})
    add_includedirs("Source/Engine/Platform/Input", {public=true})
    add_includedirs("Source/Engine/Platform/Timer", {public=true})
    add_includedirs("Source/Engine/Platform/Window", {public=true})
    add_includedirs("Source/Engine/Scene", {public=true})
    add_includedirs("Source/Engine/UI", {public=true})

    add_packages(
        "quill",
        "benchmark",
        "nlohmann_json",
        "stb",
        "tbb",
        "entt",
        "imgui",
        "d3d12-memory-allocator",
        -- "directxmesh",
        "directxmath",
        "directx-headers",
        "GameInput",
        "PIXEventRuntime")
end)

target("AcrylicLauncher", function ()
    set_kind("binary")
    set_default(true)
    add_deps("AcrylicEngine")

    add_rules("CopyD3D12AgilitySDK")

    add_packages("directx12-agility")

    add_files("Source/Launcher/**.cpp")
end)