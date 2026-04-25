cmake_minimum_required(VERSION 3.11)
add_library(SH1106_I2C)
target_sources(SH1106_I2C
    PRIVATE
        Src/sh1106.c
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            Inc
)

# set_source_files_properties(
#         Src/sh1106.c
#     PROPERTIES
#         COMPILE_OPTIONS "-O3"
# )

target_link_libraries(SH1106_I2C
    stm32cubemx
)