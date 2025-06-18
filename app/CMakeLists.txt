#cmake_minimum_required(VERSION 3.13.1)
#find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

#project(app LANGUAGES C CXX) # Keep C CXX as you have .cpp files

# Add global include directories for your application sources
# Note: Using target_include_directories is generally preferred over include_directories
#target_include_directories(app PRIVATE
#    ${CMAKE_CURRENT_SOURCE_DIR} # This refers to your app/ directory
#    ${CMAKE_CURRENT_SOURCE_DIR}/include
#    #${CMAKE_CURRENT_SOURCE_DIR}/tflite-model
#    ${CMAKE_CURRENT_SOURCE_DIR}/model-parameters
#    # ADD THIS LINE: Point to the project's top-level include directory
#    ${WEST_TOPDIR}/include
#)


#target_sources(app PRIVATE
#        src/main.c
#        src/sensor_acquisition.c
#        src/inference.cpp
#        )

#target_sources_ifdef(CONFIG_DISPLAY app PRIVATE
#        src/gui.c
#        src/gui_screens/live_chart.c
#        )

#target_sources_ifdef(CONFIG_APP_MQTT app PRIVATE
#        src/mqtt.c
#        )

#include(edge-impulse-sdk/cmake/utils.cmake)

# Add the Edge Impulse SDK
#add_subdirectory(edge-impulse-sdk/cmake/zephyr)

# Add model sources
#target_sources(app PRIVATE
#    tflite-model/tflite_learn_143_compiled.cpp
#    )

# The old 'set(INCLUDES ...)' and 'include_directories(${INCLUDES})' block should be removed.
# set(INCLUDES
#     .
#     include
#     tflite-model
#     model-parameters
#     )
# include_directories(${INCLUDES})

# Link with Zephyr interface library (provides Zephyr headers, module headers, etc.)
#target_link_libraries(app PRIVATE zephyr_interface)

#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE
        src/main.c
        src/sensor_acquisition.c
        src/inference.cpp
        )

target_sources_ifdef(CONFIG_DISPLAY app PRIVATE
        src/gui.c
        src/gui_screens/live_chart.c
        )

target_sources_ifdef(CONFIG_APP_MQTT app PRIVATE
        src/mqtt.c
        )

include(edge-impulse-sdk/cmake/utils.cmake)

# Add the Edge Impulse SDK
add_subdirectory(edge-impulse-sdk/cmake/zephyr)

# Add model sources
target_sources(app PRIVATE
    tflite-model/tflite_learn_143_compiled.cpp
    )

# Edge impulse SDK include directories
set(INCLUDES
    .
    include
    tflite-model
    model-parameters
    )
include_directories(${INCLUDES})