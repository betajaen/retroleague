solution "Retroleague"

      --------------------------------------------------------------------------

      configurations      { "Debug", "Release" }

      targetdir           "bin"
      debugdir            "bin"

      configuration "Debug"
        defines           { "DEBUG", "_CRT_SECURE_NO_WARNINGS" }
        flags             { "Symbols" }

      configuration "Release"
        defines           { "NDEBUG", "_CRT_SECURE_NO_WARNINGS"}
        flags             { "Optimize" }

      --------------------------------------------------------------------------

      project "Retroleague"
          kind            "ConsoleApp"
          language        "C"
          objdir          "_build"
          flags           { "WinMain", "FatalWarnings" }
          defines         { }
          links           { "SDL2", "SDL2main", "SDL2_net" }
          includedirs     { "ref/SDL2/include",  "ref/SDL2_net/include", "ref/" }
          libdirs         { "ref/SDL2/lib/x86/", "ref/SDL2_net/lib/x86/",  }

          files           { "TODO.md", "*.c", "*.h", "ref/*.c", "ref/*.h", "resources.rc", "resources.rc", "assets/*.png", "assets/*.wav", "assets/*.mod" }
          excludes        { "ref/*.c", "ref/*.h" }

      --------------------------------------------------------------------------

      startproject "Retroleague"

      --------------------------------------------------------------------------
