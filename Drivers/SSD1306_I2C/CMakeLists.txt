cmake_minimum_required(VERSION 3.11)
add_library(SSD1306_I2C)
target_sources(SSD1306_I2C
    PRIVATE
        Src/ssd1306.c
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            Inc
)

# set_source_files_properties(
#         Src/ssd1306.c
#     PROPERTIES
#         COMPILE_OPTIONS "-O3"
# )

target_link_libraries(SSD1306_I2C
    stm32cubemx
)