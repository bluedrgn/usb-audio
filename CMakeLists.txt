cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME usb-audio)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
add_subdirectory(Middlewares/CMSIS-DSP)
add_subdirectory(Middlewares/microGL)
# add_subdirectory(Drivers/SH1106_I2C)
add_subdirectory(Drivers/SSD1306_I2C)
# add_subdirectory(Drivers/GU128X32D)
add_subdirectory(Drivers/I2S_audio_player)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/usb_audio_class.c
    Core/Src/visuals/VUmeter.c
    Core/Src/visuals/bouncing_bars.c
    Core/Src/visuals/waveform.c
    Core/Src/filter_helpers.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    CMSISDSP
    microGL
    # SH1106_I2C
    SSD1306_I2C
    # GU128X32D
    I2S_audio_player
    m
    # Add user defined libraries
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:-u _printf_float>
)

target_include_directories(CMSISDSP PRIVATE
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include 
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
)

target_compile_definitions(CMSISDSP PUBLIC
    ARM_MATH_CM4
)

target_compile_options(CMSISDSP PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -O3
    -g0
)

set_source_files_properties(
        Core/Src/visuals/VUmeter.c
        Core/Src/visuals/bouncing_bars.c
        Core/Src/visuals/waveform.c
    PROPERTIES
        COMPILE_OPTIONS "-O3"
)
