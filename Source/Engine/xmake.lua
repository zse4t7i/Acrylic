target("AcrylicEngine", function ()
    set_kind("static")
    set_pcxxheader("PCH.hpp")

    add_rules("CopyAcrylicAsset")
    
    add_syslinks("user32", "d3d12", "dxgi")

if is_mode("release") then
    add_defines("QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO")
end
    add_defines("QUILL_DISABLE_NON_PREFIXED_MACROS")
    add_defines("D3D12MA_USING_DIRECTX_HEADERS")

    add_files("**.cpp")

    add_includedirs("./", {public=true})
    add_includedirs("Asset", {public=true})
    add_includedirs("Core", {public=true})
    add_includedirs("Graphics", {public=true})
    add_includedirs("Platform/Input", {public=true})
    add_includedirs("Platform/Timer", {public=true})
    add_includedirs("Platform/Window", {public=true})
    add_includedirs("Scene", {public=true})
    add_includedirs("UI", {public=true})

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
