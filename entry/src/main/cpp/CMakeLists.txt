# D2R Mobile HOS — native 构建脚本
# 产物 libdmhost.so:平台 HAL + 输入 + guest 加载器 + 图形后端接口 +
# EGL/GLES3 渲染循环 + NAPI 桥。对应上游 dm/host/Makefile 的角色。
cmake_minimum_required(VERSION 3.5.0)
project(d2r_mobile_hos C CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${NATIVERENDER_ROOT_PATH})

add_library(dmhost SHARED
    napi/dm_napi.cpp
    hal/dm_platform.c
    input/dm_input.c
    loader/dm_guest_loader.c
    loader/dm_guest_placeholder.S
    gfx/dm_gfx_backend.c
    render/dm_renderer.cpp
)

target_link_libraries(dmhost PUBLIC
    libace_napi.z.so
    libace_ndk.z.so
    libEGL.so
    libGLESv3.so
    libhilog_ndk.z.so
    libvulkan.so
)
