-- tracy

tracy_includedir = path.getabsolute("tracy-0.8")
tracy_files = {
	tracy_includedir .. "/*.hpp",
	tracy_includedir .. "/common/*.hpp",
	tracy_includedir .. "/client/*.hpp",
	tracy_includedir .. "/TracyClient.cpp",
}


    