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
