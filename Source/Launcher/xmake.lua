target("AcrylicLauncher", function ()
    set_kind("binary")
    set_default(false)
    add_deps("AcrylicEngine")

    add_files("Script/**.cpp")
end)