rule("CopyResource")
    before_build(function (target)
        os.cp("$(scriptdir)/Font/", target:targetdir(), {copy_if_different = true})

        cprint("${bright green}Engine resources copied!")
    end)

rule("CopyD3D12AgilitySDK")
    before_build(function (target)
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

target("Acrylic", function ()
    set_kind("binary")
    set_pcxxheader("Source/PCH.hpp")
    add_deps("Project")

    add_rules("CopyResource")
    add_rules("CopyD3D12AgilitySDK")
    
    add_cxxflags("-fp:fast")
    add_syslinks("user32", "d3d12", "dxgi")

    add_files("Source/**.cpp")

    add_includedirs("Source/Core")
    add_includedirs("Source/Util")

    add_packages("quill")
    add_packages("benchmark")
    add_packages("nlohmann_json")
    add_packages("stb")
    add_packages("tbb")
    add_packages("entt")
    add_packages("tinygltf")
    add_packages("imgui")
    add_packages("d3d12-memory-allocator")
    add_packages("directxtex")
    -- add_packages("directxmesh")
    add_packages("directxmath")
    add_packages("directx-headers")
    add_packages("directx12-agility")
end)
