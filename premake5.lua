workspace "nextminer"
    configurations {"Debug", "Release"}
    language "C++"
    cppdialect "C++14"
    
    filter {"configurations:Debug"}
        optimize "Off"
        symbols "On"

    filter {"configurations:Release"}
        optimize "Full"
        symbols "Off"

project "nextminer"
        
    files {"src/**.cpp", "src/**.h", "src/**.c", "src/**.hpp"}
    kind "ConsoleApp"
    links {"jsoncpp", "sfml-network", "sfml-system", "crypto"}

    filter "system:linux"
        links {"pthread"}