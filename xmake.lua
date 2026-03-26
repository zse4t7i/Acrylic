set_project("Acrylic")
set_version("0.0.1", {build = "%Y%m%d%H%M"})

set_policy("build.progress_style", "multirow")
set_xmakever("3.0.7")
set_encodings("utf-8")
set_languages("c++20")
set_toolchains("msvc")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

if is_mode("debug") then
    add_defines("DEBUG", "_DEBUG")
    add_defines("QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG")
elseif is_mode("release") then
    add_defines("NDEBUG")
    add_defines("RELEASE")
    add_defines("QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO")
end

add_defines("QUILL_DISABLE_NON_PREFIXED_MACROS")
-- Win32 define
add_defines("UNICODE", "_UNICODE")
add_defines("NOMINMAX")
add_defines("NODRAWTEXT", "NOGDI", "NOBITMAP")
add_defines("NOMCX", "NOSERVICE", "NOHELP")
add_defines("WIN32_LEAN_AND_MEAN")

add_requires("benchmark", "quill", "nlohmann_json",
            "imgui[dx12,win32]",
            "assimp", "stb", "fastgltf",
            "directx12-agility", "d3d12-memory-allocator",
            "directxtk12", "directxtex", "directxmath")

rule("AssetCopy")
    before_build(function (target)
        os.cp("Asset/*", "bin/Asset/")
        cprint("${bright green}Asset copied!")
    end)

rule("ShaderCompile")
    set_extensions(".hlsl")
    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        -- Determine shader profile based on file name convention
        -- (e.g., file.vs.hlsl, file.ps.hlsl)
        local filename = path.filename(sourcefile)
        local shaderProfile = "vs_6_9"
        
        if filename:match("%.vs%.hlsl$") then
            shaderProfile = "vs_6_9"
        elseif filename:match("%.ps%.hlsl$") then
            shaderProfile = "ps_6_9"
        elseif filename:match("%.cs%.hlsl$") then
            shaderProfile = "cs_6_9"
        end

        -- Output to bin/Shader
        local outputDir = path.join(os.projectdir(), target:targetdir(), "Shader")
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
            opt.progress, "${color.build.object}compiling.HLSL %s", sourcefile)
    end)

target("Acrylic", function ()
    set_kind("binary")
    set_targetdir("bin")

    add_rules("AssetCopy")
    add_rules("ShaderCompile")
    
    add_cxflags("-arch:AVX2", "-fp:fast")
    add_syslinks("user32", "d3d12", "dxgi", "dxguid")

    add_files("Source/**.cpp")
    add_files("Shader/**.hlsl")

    add_includedirs("Source/Core")
    add_includedirs("Source/Util")

    add_packages("benchmark", "quill", "nlohmann_json",
                "imgui[dx12,win32]",
                "assimp", "stb", "fastgltf",
                "directx12-agility", "d3d12-memory-allocator",
                "directxtk12", "directxtex", "directxmath")
end)
