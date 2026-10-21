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
    host/hle_libc.c
    host/hle_sdkinit.c
    host/hle_fs.c
    host/hle_oe.c
    host/hle_hid.c
    host/hle_time.c
    host/hle_misc.c
    host/hle_account.c
    host/hle_friends.c
    host/hle_audio_srv.c
    host/hle_os_event.c
    host/hle_net.c
    host/hle_vi.c
    host/loader_hos.c
    host/front_hos.c
    host/host_boot.c
    host/host_start.c
    host/crash_hos.c
    gfx/dm_gfx_backend.c
    gfx/dm_vk.cpp
    gfx/nvn/dm_nvn.c
    render/dm_renderer.cpp
    audio/dm_audio.c
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    # ARM64:门跳汇编 + 内嵌 guest 模块(hos_embed.py 产物,绝不入库);
    # 镜像在 "ax" 段原地执行(数据窗口由 loader mprotect RW 后原地重定位)
    target_sources(dmhost PRIVATE
        host/entry_hos.S
        loader/dm_guest_placeholder.S
        gfx/nvn/dm_nvn_stubs.S
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
    libnative_window.so
    libvulkan.so
    libz.so
    libohaudio.so
)

# 隐式函数声明一律报错:guest 导入解析返回 uint64_t,漏声明会按 int 截断成
# 32 位(真机表现为跳到 0x7d19ed4 这种半截地址、SIGSEGV@低地址)。
target_compile_options(dmhost PRIVATE -Werror=implicit-function-declaration)

# 独立的 XComponent napi 模块:libdmhost.so 被 ArkTS import 先初始化后,
# XComponent 复用同一模块不再调 Init,导致原生 surface 回调注册不上。
# 这个 .so 只由 XComponent(libraryname="dmxe")加载,专门注册回调并调
# libdmhost 导出的 dmVkStart/dmVkStop。
add_library(dmxe SHARED napi/dm_xc_napi.cpp)
target_link_libraries(dmxe PRIVATE
    libace_napi.z.so
    libace_ndk.z.so
    libhilog_ndk.z.so
    libnative_window.so
    dl
)


