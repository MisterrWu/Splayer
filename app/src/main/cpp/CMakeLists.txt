cmake_minimum_required(VERSION 3.4.1)

set(libs_DIR ${CMAKE_SOURCE_DIR}/../../../../prebuild/${ANDROID_ABI}/lib)
set(include_DIR ${CMAKE_SOURCE_DIR}/../../../../prebuild/${ANDROID_ABI}/include)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})

add_library(avcodec STATIC IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION
    ${libs_DIR}/libavcodec.a)

#add_library(avfilter STATIC IMPORTED)
#set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION
#    ${libs_DIR}/libavfilter.a)

add_library(avformat STATIC IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION
    ${libs_DIR}/libavformat.a)

add_library(avutil STATIC IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION
    ${libs_DIR}/libavutil.a)

add_library(swresample STATIC IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION
    ${libs_DIR}/libswresample.a)

add_library(swscale STATIC IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION
    ${libs_DIR}/libswscale.a)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -D__STDC_CONSTANT_MACROS
                                     -D__STDC_LIMIT_MACROS
                                     -O3
                                     -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wall -v -Wl,--no-warn-shared-textrel")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall")

add_library(splayer SHARED
            audio_device_opensles.cpp
            video_render_opengles20.cpp
            video_render_android.cpp
            player_helper.cpp
            #player_file.cpp
            #player_log.cpp
            player.cpp
            #player_hard.cpp
            player_soft.cpp
            player_jni_entry.cpp)

target_include_directories(splayer PUBLIC
                           ${CMAKE_SOURCE_DIR}
                           ${include_DIR}/../
                           ${include_DIR})

target_link_libraries(splayer
                      log
                      GLESv2
                      OpenSLES
                      m
                      z
                      avcodec
                      avformat
                      avutil
                      swresample
                      swscale)