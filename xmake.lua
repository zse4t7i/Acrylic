set_project("Acrylic")
set_version("0.0.1", {build = "%Y%m%d%H%M"})

set_policy("build.progress_style", "multirow")
set_xmakever("3.0.8")
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
add_defines("D3D12MA_USING_DIRECTX_HEADERS")
-- Win32 define
add_defines("UNICODE", "_UNICODE")
add_defines("NOMINMAX")
add_defines("NODRAWTEXT", "NOGDI", "NOBITMAP")
add_defines("NOMCX", "NOSERVICE", "NOHELP")
add_defines("WIN32_LEAN_AND_MEAN")

add_requires("quill")
add_requires("benchmark")
add_requires("nlohmann_json")
add_requires("stb")
add_requires("tinygltf")
add_requires("imgui[dx12,win32]")
add_requires("d3d12-memory-allocator")
add_requires("directxtex")
-- add_requires("directxmesh")
add_requires("directxmath")
add_requires("directx-headers")
add_requires("directx12-agility")

rule("AssetCopy")
    before_build(function (target)
        os.cp("Asset/*", "bin/Asset/")

        cprint("${bright green}Asset copied!")
    end)

rule("D3D12AgilitySDKCopy")
    before_build(function (target)
        local srcDir = target:pkg("directx12-agility"):installdir()
        local dstDir = path.join(target:targetdir(), "D3D12")

        -- Copy Agility SDK DLLs from directx12-agility package to the bin/D3D12 directory
        os.cp(path.join(srcDir, "bin", "D3D12Core.dll"), path.join(dstDir, "D3D12Core.dll"))
        if is_mode("debug") then
            os.cp(path.join(srcDir, "bin", "d3d12SDKLayers.dll"), path.join(dstDir, "d3d12SDKLayers.dll"))
        end

        cprint("${bright green}D3D12 Agility SDK copied!")

        -- Parse the version number from the directx12-agility package and define D3D12_AGILITY_SDK_VERSION
        local version = target:pkg("directx12-agility"):version():minor()
        target:add("defines", "D3D12_AGILITY_SDK_VERSION=" .. version)
    end) 

rule("ShaderCompile")
    set_extensions(".hlsl")
    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        -- Determine shader profile based on file name convention
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

        -- Output to bin/Shader
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
            opt.progress, "${color.build.object}Compiling .HLSL %s", sourcefile)
    end)

target("Acrylic", function ()
    set_kind("binary")
    set_targetdir("bin")

    add_rules("AssetCopy")
    add_rules("D3D12AgilitySDKCopy")
    add_rules("ShaderCompile")
    
    add_cxxflags("-fp:fast")
    add_syslinks("user32", "d3d12", "dxgi")

    add_files("Source/**.cpp")
    add_files("Shader/**.hlsl")

    add_includedirs("Source/Core")
    add_includedirs("Source/Util")

    add_packages("quill")
    add_packages("benchmark")
    add_packages("nlohmann_json")
    add_packages("stb")
    add_packages("tinygltf")
    add_packages("imgui[dx12,win32]")
    add_packages("d3d12-memory-allocator")
    add_packages("directxtex")
    -- add_packages("directxmesh")
    add_packages("directxmath")
    add_packages("directx-headers")
    add_packages("directx12-agility")
end)
