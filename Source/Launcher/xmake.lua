target("AcrylicLauncher", function ()
    set_kind("binary")
    set_default(true)
    add_deps("AcrylicEngine")

    add_files("**.cpp")
end)