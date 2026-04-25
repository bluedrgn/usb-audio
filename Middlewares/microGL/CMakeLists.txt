cmake_minimum_required(VERSION 3.11)
add_library(microGL)
target_sources(microGL
	PRIVATE
		Src/microGL.c
		Src/fonts.c
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS
			Inc
)
# set_source_files_properties(
# 		Src/microGL.c
#   	PROPERTIES
# 		COMPILE_OPTIONS "-O3"
# )
