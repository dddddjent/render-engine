add_rules("mode.release", "mode.debug")

add_requires("vulkansdk", "glfw")
add_requires("imgui",  {configs = {glfw_vulkan = true}})

target("imgui_vulkan")
    if is_plat("windows") then
        add_rules("plugin.vsxmake.autoupdate")
        add_cxxflags("/utf-8")
    end
    set_kind("static")
    set_languages("c++17")
    add_headerfiles("*.h")
    add_files("*.cpp")
    add_packages("imgui", "vulkansdk", "glfw", "glm")
    add_includedirs(".",{public=true})
    add_includedirs("./../config/",{public=true})
    add_includedirs("./../vulkan_engine/",{public=true})
    add_deps("util")

