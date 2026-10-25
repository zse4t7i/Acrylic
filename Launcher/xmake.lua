target("AcrylicLauncher", function ()
    set_kind("binary")
    set_default(true)
    add_deps("AcrylicEngine")

    add_rules("CopyD3D12AgilitySDK")

    add_packages("directx12-agility")

    add_files("**.cpp")
end)