rule("CopyFont")
    before_build(function (target)
        os.cp("$(scriptdir)/Font/", target:targetdir(), {copy_if_different = true})

        cprint("${bright green}Engine's resources copied!")
    end)

target("AcrylicEngine", function ()
    set_kind("static")
    set_pcxxheader("Source/PCH.hpp")

    add_rules("CopyFont")
    
    add_cxxflags("-fp:fast")
    add_syslinks("user32", "d3d12", "dxgi")

if is_mode("release") then
    add_defines("QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO")
end
    add_defines("QUILL_DISABLE_NON_PREFIXED_MACROS")
    add_defines("D3D12MA_USING_DIRECTX_HEADERS")

    add_files("Source/**.cpp")

    add_includedirs("Source/", {public=true})
    add_includedirs("Source/Asset", {public=true})
    add_includedirs("Source/Core", {public=true})
    add_includedirs("Source/Graphics", {public=true})
    add_includedirs("Source/Platform/Input", {public=true})
    add_includedirs("Source/Platform/Timer", {public=true})
    add_includedirs("Source/Platform/Window", {public=true})
    add_includedirs("Source/Scene", {public=true})
    add_includedirs("Source/UI", {public=true})

    add_packages("quill")
    add_packages("benchmark")
    add_packages("nlohmann_json")
    add_packages("stb")
    add_packages("tbb")
    add_packages("entt")
    add_packages("imgui")
    add_packages("d3d12-memory-allocator")
    -- add_packages("directxmesh")
    add_packages("directxmath")
    add_packages("directx-headers")
    add_packages("GameInput")
    add_packages("PIXEventRuntime")
end)
