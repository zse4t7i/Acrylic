target("AcrylicEditor", function ()
    set_kind("binary")
    set_default(false)
    add_deps("AcrylicEngine")
    
    add_rules("CopyD3D12AgilitySDK")

    add_packages("directx12-agility")
    add_packages("directxtex")
    add_packages("tinygltf")

    add_files("**.cpp")
end)