add_rules("mode.debug", "mode.release")
includes("@builtin/xpack")

if is_plat("windows") then
    if is_mode("debug") then
        set_runtimes("MTd")
    else
        set_runtimes("MT")
    end
end

if is_mode("release") then
    set_policy("build.optimization.lto", true)
end

add_requires("fmt")
add_requires("lief")
add_requires("ftxui", {config = {modules = true}})
add_requires("glaze")
add_requires("boost", {system = false, configs = {unordered = true, leaf = true}})

target("dyneeded_core")
    set_languages("cxx23")
    set_kind("static")

    add_files("core/**.cpp")
    add_headerfiles("core/**.hpp")

    add_packages("fmt", "boost", "lief", "ftxui", "glaze", { public = true })
    if is_plat("linux") then
        add_defines("DYNEEDED_LINUX", { public = true })
    elseif is_plat("windows") then
        add_cxxflags("/utf-8", { public = true }) -- for fmt
        add_defines("DYNEEDED_WINDOWS", "NOMINMAX", { public = true })
    end

    add_includedirs(".", { public = true })

target("dyneeded")
    set_languages("cxx23")
    set_kind("binary")

    add_files("src/**.cpp")
    add_headerfiles("src/**.hpp")

    add_deps("dyneeded_core")

xpack("dyneeded")
    set_version("1.0.0")
    set_homepage("https://github.com/KingdomOfCpp/dyneeded")
    add_targets("dyneeded")

    set_title("dyneeded")
    set_description("Cross platform dynamic dependency CLI tool")
    set_author("KingdomOfCpp")

    set_formats("nsis", "zip", "targz", "runself")