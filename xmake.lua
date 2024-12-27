add_rules("mode.release", "mode.debug")

add_requires("vulkansdk", "glfw 3.4", "glm 1.0.1")
add_requires("glslang 1.3", { configs = { binaryonly = true } })
add_requires("imgui 1.91.1",  {configs = {glfw_vulkan = true}})
add_requires("cuda", {system=true, configs={utils={"cublas","cusparse","cusolver"}}})
add_requires("spdlog 1.14.1")
add_repositories("my-repo ./../../myrepo", {rootdir = os.scriptdir()})
add_requires("ffmpeg 7.0")
add_requires("boost 1.85.0")
add_requires("libtiff 4.7.0")
add_requires("libpng 1.6.44")
add_requires("libjpeg-turbo 3.0.4")
add_requires("tinyobjloader fe9e7130a0eee720a28f39b33852108217114076")
add_requires("nlohmann_json")
add_requires("vtk 9.3.1")

target("engine")
    if is_plat("windows") then
        add_rules("plugin.vsxmake.autoupdate")
        add_cxxflags("/utf-8")
    end

    set_languages("cxx20")
    set_kind("static")

    add_files("**.cpp")
    add_files("**.cu")
    add_includedirs(".",{public=true})
    add_headerfiles("./function/render/render_engine.h")
    add_headerfiles("./function/physics/cuda_engine.h")
    add_headerfiles("./function/ui/imgui_engine.h")
    add_headerfiles("./function/script/script.h")

    add_cugencodes("compute_75")
    add_cuflags("--std c++20", "-lineinfo")

    add_packages("imgui", {public = true})
    add_packages("vulkansdk", "glfw", "glm")
    add_packages("cuda",{public=true})
    add_packages("glslc")
    add_packages("spdlog", {public=true})
    add_packages("ffmpeg")
    add_packages("boost", {public=true})
    add_packages("tinyobjloader")
    add_packages("libtiff", "libpng", "libjpeg-turbo")
    add_packages("nlohmann_json")
    add_packages("vtk")

    if is_mode("debug") then
        add_cxxflags("-DDEBUG")
    elseif is_mode("release") then
        add_cxxflags("-DNDEBUG")
    end
    add_cuflags("--diag-suppress=20012")
    before_build(function (target)
        os.mkdir("$(buildir)/shaders")
    end)
    after_build(function (target)
        os.cp("$(scriptdir)/function/render/render_graph/shader/*.glsl", "$(buildir)/shaders")
    end)

local function shader_target(name)
target(name.. "_shader")
    set_kind("static")
    add_rules("utils.glsl2spv",
        {
            outputdir = "$(buildir)/shaders/" .. name,
            debugsource = is_mode("debug")
        })
    add_packages("glslc")
    add_files("**/node/"..name.."/*.vert")
    add_files("**/node/"..name.."/*.frag")
    after_build(function (target)
        if not is_mode("release") then
            return
        end
        import("lib.detect.find_program")
        local spirv_opt = find_program("spirv-opt")
        if spirv_opt == nil then
            print("spirv-opt not found, no shader optimization will be performed")
            return
        end
        local folder = "$(buildir)/shaders/" .. name
        for _, filepath in ipairs(os.files(folder .. "/*.spv")) do
            os.runv(spirv_opt, {filepath, "-o", filepath .. ".opt", "-O"})
            os.runv("rm", {filepath})
            os.runv("mv", {filepath .. ".opt", filepath})
        end
    end)
end

shader_target("default_object")
shader_target("fire_object")
shader_target("smoke_field")
shader_target("vorticity_field")
shader_target("fire_field")
shader_target("hdr_to_sdr")
