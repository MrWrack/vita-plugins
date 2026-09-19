cmake_minimum_required(VERSION 3.10)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
 if(DEFINED ENV{VITASDK})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "")
 else()
  message(FATAL_ERROR "VITASDK is not set")
 endif()
endif()
project(VitaAutoPluginBackground C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

add_executable(${PROJECT_NAME} src/main.c)
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wall)
target_link_libraries(${PROJECT_NAME}
 SceCtrl_stub
 SceKernelThreadMgr_stub
 SceLibKernel_stub
)
vita_create_self(VitaAutoPluginBackground.suprx ${PROJECT_NAME}
 UNSAFE CONFIG ${CMAKE_SOURCE_DIR}/exports.yml)
