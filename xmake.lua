-- project and version
set_xmakever("2.2.3")
set_project("SpinlockTest")


-- add project directories or targets
target("SpinlockTest")

    -- add rules
    add_rules("mode.debug")
    add_rules("mode.release")

    -- set kind
    set_kind("binary")

    -- add files
    add_files("src/**.cpp")
    add_headerfiles("inc/**.h")

    -- add include search directories
    add_includedirs("inc", {public = true})
