target("Launcher", function ()
    set_kind("binary")
    set_default(true)
    add_deps("Engine")

    add_files("**.cpp")
end)