#
#  I2S Audio Player
#    STM32 module
#  
#    Created on: Apr 10, 2026
#        Author: bluedrgn
#  

cmake_minimum_required(VERSION 3.11)
add_library(I2S_audio_player)
target_sources(I2S_audio_player
    PRIVATE
        Src/audio_player.c
        Src/resample.c
    PRIVATE 
        FILE_SET priv_headers
        TYPE HEADERS
        BASE_DIRS
            PrivateInc
    PUBLIC
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS
            Inc
)

set_source_files_properties(
        Src/resample.c
    PROPERTIES
        COMPILE_OPTIONS "-O3"
)

target_link_libraries(I2S_audio_player PRIVATE
    stm32cubemx
    CMSISDSP
    m
)