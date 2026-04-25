cmake_minimum_required(VERSION 3.11)
add_library(I2S_audio_player)
target_sources(I2S_audio_player
    PRIVATE
        Src/audio_player.c
        Src/resample.c
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            Inc
)

set_source_files_properties(
        Src/resample.c
    PROPERTIES
        COMPILE_OPTIONS "-O3"
)

target_link_libraries(I2S_audio_player
    stm32cubemx
)