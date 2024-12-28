function shader_target(name)
    target(name .. "_shader")
    set_kind("static")
    add_rules("utils.glsl2spv",
        {
            outputdir = "$(buildir)/shaders/" .. name,
            debugsource = is_mode("debug")
        })
    add_packages("glslc")
    add_files("**/node/" .. name .. "/*.vert")
    add_files("**/node/" .. name .. "/*.frag")
    after_build(function(target)
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
            os.runv(spirv_opt, { filepath, "-o", filepath .. ".opt", "-O" })
            os.runv("rm", { filepath })
            os.runv("mv", { filepath .. ".opt", filepath })
        end
    end)
end
