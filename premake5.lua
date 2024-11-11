workspace "Slicer"
	configurations { "Debug", "Release" }
	architecture "x86_64"

	outputdir = "%{cfg.system}/%{cfg.buildcfg}"
	targetdir ("%{wks.location}/bin/" .. outputdir)
	objdir ("%{wks.location}/obj/" .. outputdir)

	startproject "Slicer"

	filter "system:macosx"
		links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework", "CoreVideo.framework", "QuartzCore.framework" }
		
	filter""

	include "vendor/Nexus"
	include "vendor/Nexus/vendor/premake5_glfw.lua"
	include "vendor/Nexus/vendor/premake5_ImGui.lua"
	include "vendor/Nexus/vendor/Glad"

project "Slicer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	-- prebuildcommands {

	-- 	"cd vendor/assimp && cmake CMakeLists.txt -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../bin/{cfg.system}/{cfg.buildcfg} && cmake --build . && cd ../../"
	-- }

	includedirs {
		"src",
		"include",
		"vendor",
		"vendor/Nexus/include",
		"vendor/Nexus/vendor",
		"vendor/Nexus/vendor/Glad/include",
		"vendor/Nexus/vendor/GLFW/include",
		"vendor/Nexus/vendor/glm/glm",
		"vendor/Nexus/vendor/ImGui",
		"vendor/Nexus/vendor/spdlog/include",
	}

	files { 
		"include/**.h",
		"src/**.cpp",
		"vendor/Nexus/vendor/glm/glm/**.hpp",
		"vendor/Nexus/vendor/glm/glm/**.inl",
	}

	links {
		"Nexus",
		"glfw",
		"Glad",
		"ImGui",
		"assimp"
	}

	libdirs {
		os.findlib("assimp")
	}
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
	
	filter ""