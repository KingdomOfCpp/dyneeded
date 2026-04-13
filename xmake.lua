add_rules("mode.debug", "mode.release")

add_requires("fmt")
add_requires("lief")
add_requires("fast_io")
add_requires("boost", {configs = {unordered = true, leaf = true}})

target("dyneeded_core")
    set_languages("cxx20")
    set_kind("static")

    add_files("core/**.cpp")
    add_headerfiles("core/**.hpp")

    add_packages("fmt", "fast_io", "boost", "lief", { public = true })
    if is_plat("linux") then
        add_defines("DYNEEDED_LINUX", { public = true })
    end

target("dyneeded")
    set_languages("cxx20")
    set_kind("binary")

    add_files("src/**.cpp")
    add_headerfiles("src/**.hpp")

    add_deps("dyneeded_core")