solution "Craig2kmlSolution" 
	configurations { "Debug", "Release" }
	
-- A project defines one build target
project "craig2kml"
	kind "ConsoleApp"
	language "C++"
	files { "src/*.h", "src/*.cpp" }
	links { 
		"xml2", "tidy", "curl", "z", "pthread", "iconv", "m",
		"kmlbase", "kmlconvenience", "kmldom", "kmlengine" }
	libdirs { 
		"/opt/local/lib", 
		"/usr/local/lib", 
		os.findlib("kmlbase"),
		os.findlib("tidy"),
		os.findlib("xml2"), 
		os.findlib("curl")   }
	includedirs { 
		"/opt/local/include",
		"/opt/local/include/libxml2",
		"/usr/local/include/tidy",
		"/usr/local/include/kml/**" }
	
configuration "Debug"
	defines { "DEBUG" }
	flags { "Symbols" }
	
configuration "Release"
	defines { "NDEBUG" }
	flags { "Optimize" }  