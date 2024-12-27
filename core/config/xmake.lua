add_rules("mode.release", "mode.debug")

add_requires("nlohmann_json")

target("config")
    add_rules("plugin.vsxmake.autoupdate")
    set_kind("static")
    set_languages("c++17")
    add_headerfiles("*.h")
    add_files("*.cpp")
    add_includedirs(".",{public=true})
    add_packages("nlohmann_json")
