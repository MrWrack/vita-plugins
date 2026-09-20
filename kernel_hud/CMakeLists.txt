cmake_minimum_required(VERSION 3.10)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "")
  else()
    message(FATAL_ERROR "VITASDK is not set")
  endif()
endif()
project(VitaAutoPluginHUD C)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
add_executable(${PROJECT_NAME} src/main.c)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_EXTENSIONS ON)
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wall -Wextra)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-nostdlib -Wl,-e,module_start")
target_link_libraries(${PROJECT_NAME}
  gcc
  taihenForKernel_stub
  SceThreadmgrForDriver_stub
  SceSysmemForDriver_stub
  ScePowerForDriver_stub
  SceCtrlForDriver_stub
)
vita_create_self(VitaAutoPluginHUD.skprx ${PROJECT_NAME} UNSAFE CONFIG ${CMAKE_SOURCE_DIR}/exports.yml)
