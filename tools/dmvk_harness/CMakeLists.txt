cmake_minimum_required(VERSION 3.16)
project(dmvk_harness CXX C)
set(CMAKE_CXX_STANDARD 17)
# 桌面 Vulkan 回放校验:读设备导出的 f000.dump,用同一批离线 SPIR-V 建管线,
# 打开 Khronos Validation Layer(VVL),把 Maleoon 上只报 OOM 的问题变成 VUID 明细。
find_package(Vulkan REQUIRED)
add_executable(dmvk_harness main.cpp)
target_link_libraries(dmvk_harness PRIVATE Vulkan::Vulkan)
target_compile_definitions(dmvk_harness PRIVATE DMVK_HARNESS=1)
