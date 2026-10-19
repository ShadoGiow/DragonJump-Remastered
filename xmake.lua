-- Progetto PERSONALE (uso privato dell'autore, mashup di funzioni di varie mod che mi piacciono).
-- Non e' una release pensata per altre persone.
set_xmakever("3.0.0")

-- libreria di base del plugin, clonata dal workflow in lib/commonlibsse-ng
includes("lib/commonlibsse-ng")

set_project("MagicalJumpingNative")
set_version("2.0.0")
set_languages("c++23")

add_rules("mode.debug", "mode.releasedbg")

target("MagicalJumpingNative")
    add_rules("commonlibsse-ng.plugin", {
        name = "MagicalJumpingNative",
        author = "Giovanni",
        description = "MJN 2.0 - plugin SKSE personale (mashup di funzioni di varie mod che mi piacciono), senza Papyrus. Non e' una release pubblica."
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/PCH.h")

    add_defines("NOMINMAX")
    add_cxxflags("cl::/wd4100", "cl::/wd4201", "cl::/utf-8", {force = true})

    after_build(function (target)
        local dist = path.join(os.projectdir(), "dist", "SKSE", "Plugins")
        os.mkdir(dist)
        os.cp(target:targetfile(), dist)
        local pdb = target:symbolfile()
        if pdb and os.isfile(pdb) then
            os.cp(pdb, dist)
        end
    end)
