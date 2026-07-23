target("Editor", function ()
    set_kind("binary")
    set_default(false)
    add_deps("Engine")
    add_packages("tinygltf")

    add_files("**.cpp")
end)