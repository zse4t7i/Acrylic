target("AcrylicEditor", function ()
    set_kind("binary")
    set_default(false)
    add_deps("AcrylicEngine")

    add_packages("directxtex")
    add_packages("tinygltf")

    add_files("**.cpp")
end)