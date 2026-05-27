rule("CopyResource")
    before_build(function (target)
        os.cp("$(scriptdir)/Mesh/", target:targetdir(), {copy_if_different = true})
        os.cp("$(scriptdir)/Texture/", target:targetdir(), {copy_if_different = true})

        cprint("${bright green}Project resources copied!")
    end)

rule("CompileShader")
    set_extensions(".hlsl")
    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        -- Determine shader profile based on file name convention.
        -- (e.g., file.vs.hlsl, file.ps.hlsl)
        local filename = path.filename(sourcefile)
        local shaderProfile = "vs_6_7"
        
        if filename:match("%.vs%.hlsl$") then
            shaderProfile = "vs_6_7"
        elseif filename:match("%.ps%.hlsl$") then
            shaderProfile = "ps_6_7"
        elseif filename:match("%.cs%.hlsl$") then
            shaderProfile = "cs_6_7"
        end

        -- Output to Shader/ folder in the output directory.
        local outputDir = path.join(target:targetdir(), "Shader")
        local outputBin = path.join(
            outputDir,
            path.basename(sourcefile) .. ".bin"
        )

        -- Create output directory if it doesn't exist
        os.mkdir(outputDir)

        if is_mode("debug") then
            local outputPdb = path.join(
                outputDir,
                path.basename(sourcefile) .. ".pdb"
            )

            batchcmds:vrunv("dxc", {
                "-T", shaderProfile,
                "-E", "Main",
                "-Fo", outputBin,
                "-Zi",
                "-Fd", outputPdb,
                sourcefile
            })
        elseif is_mode("release") then
            batchcmds:vrunv("dxc", {
                "-T", shaderProfile,
                "-E", "Main",
                "-Fo", outputBin,
                sourcefile
            })
        end

        batchcmds:add_depfiles(sourcefile)
        batchcmds:set_depmtime(os.mtime(outputBin))
        batchcmds:show_progress(
            opt.progress, "${color.build.object}Compiling HLSL: %s", sourcefile)
    end)

target("Project", function ()
    set_kind("shared")
    set_pcxxheader("Script/PCH.hpp")

    add_rules("CopyResource")
    add_rules("CompileShader")
    add_rules("utils.symbols.export_all", {export_classes = true})

    add_files("Script/**.cpp")
    add_files("Shader/**.hlsl")

    add_includedirs("Script", {public=true})
end)