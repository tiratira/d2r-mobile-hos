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
    host/hle_core.c
    host/hle_svc.c
    host/hle_thread.c
    host/hle_hostcall.c
    host/loader_hos.c
    host/front_hos.c
    host/host_boot.c
    gfx/dm_gfx_backend.c
    gfx/dm_vk.cpp
    gfx/nvn/dm_nvn.c
    render/dm_renderer.cpp
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    # ARM64:门跳汇编 + 内嵌 guest 模块(hos_embed.py 产物,绝不入库);
    # 镜像在 "ax" 段原地执行(数据窗口由 loader mprotect RW 后原地重定位)
    target_sources(dmhost PRIVATE
        host/entry_hos.S
        loader/dm_guest_placeholder.S
    )
    set(GUEST_S "")
    foreach(m main sdk subsdk0)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/loader/guest_${m}.S")
            list(APPEND GUEST_S "${CMAKE_CURRENT_SOURCE_DIR}/loader/guest_${m}.S")
            message(STATUS "guest module embedded: ${m} (in-place exec)")
        endif()
    endforeach()
    target_sources(dmhost PRIVATE ${GUEST_S})
else()
    # x86_64(PC 模拟器):ARM64 汇编不可编;guest 镜像以 data-only 容器嵌入,
    # 装载/重定位/符号解析路径与真机同码可测,仅不执行
    target_sources(dmhost PRIVATE
        host/entry_x86_stub.c
        loader/dm_guest_placeholder_x86.S
    )
    set(GUEST_S "")
    foreach(m main sdk subsdk0)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/loader/guest_${m}_x86.S")
            list(APPEND GUEST_S "${CMAKE_CURRENT_SOURCE_DIR}/loader/guest_${m}_x86.S")
            message(STATUS "guest module embedded (data-only): ${m}")
        endif()
    endforeach()
    target_sources(dmhost PRIVATE ${GUEST_S})
    message(STATUS "x86_64 build: guest execution disabled (ARM64-only)")
endif()

target_link_libraries(dmhost PUBLIC
    libace_napi.z.so
    libace_ndk.z.so
    libEGL.so
    libGLESv3.so
    libhilog_ndk.z.so
    libvulkan.so
    libz.so
)
