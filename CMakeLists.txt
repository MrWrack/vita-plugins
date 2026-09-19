cmake_minimum_required(VERSION 3.10)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
 if(DEFINED ENV{VITASDK})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "")
 else()
  message(FATAL_ERROR "VITASDK is not set")
 endif()
endif()
project(VitaAutoPlugin C)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_APP_NAME "Vita AutoPlugin")
set(VITA_TITLEID "MRWR00001")
set(VITA_VERSION "01.00")
add_executable(${PROJECT_NAME} src/main.c src/tai_config.c src/repository.c src/plugin_verify.c src/plugin_download.c src/package_verify.c src/download_status.c src/vita_http_transport.c src/plugin_browser_ui.c src/plugin_browser_render.c src/ui_font.c src/plugin_browser_input.c src/plugin_browser_screen.c)
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wall)
target_link_libraries(${PROJECT_NAME}
 vita2d SceCtrl_stub SceTouch_stub SceDisplay_stub SceGxm_stub ScePgf_stub
 ScePower_stub SceIofilemgr_stub SceLibKernel_stub m
)
vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(VitaAutoPlugin.vpk ${VITA_TITLEID} eboot.bin
 VERSION ${VITA_VERSION}
 NAME ${VITA_APP_NAME}
 FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
 ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
 ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
 ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
