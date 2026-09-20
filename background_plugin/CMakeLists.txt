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
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_EXTENSIONS ON)
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wall -Wextra)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-nostdlib")
target_link_libraries(${PROJECT_NAME}
  taihen_stub
  SceCtrl_stub
  SceDisplay_stub
  ScePower_stub
  SceIofilemgr_stub
  SceProcessmgr_stub
  SceKernelThreadMgr_stub
  SceLibKernel_stub
  gcc
)
vita_create_self(VitaAutoPluginBackground.suprx ${PROJECT_NAME} UNSAFE CONFIG ${CMAKE_SOURCE_DIR}/exports.yml)
