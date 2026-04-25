cmake_minimum_required(VERSION 3.11)
add_library(GU128X32D)
target_sources(GU128X32D
    PRIVATE
        Src/gu128x32d.c
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            Inc
)

# set_source_files_properties(
#         Src/gu128x32d.c
#     PROPERTIES
#         COMPILE_OPTIONS "-O3"
# )

target_link_libraries(GU128X32D
    stm32cubemx
)