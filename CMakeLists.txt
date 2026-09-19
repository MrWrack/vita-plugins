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
 vita2d
 png
 z
 SceAppMgr_stub
 SceCommonDialog_stub
 SceSysmodule_stub
 SceCtrl_stub
 SceTouch_stub
 SceDisplay_stub
 SceGxm_stub
 ScePgf_stub
 ScePvf_stub
 ScePower_stub
 SceIofilemgr_stub
 SceLibKernel_stub
 m
)

# PS Vita LiveArea PNG guard: require 8-bit indexed PNG assets.
function(psv_assert_indexed_png path)
 file(READ "${path}" _ihdr HEX LIMIT 26)
 string(SUBSTRING "${_ihdr}" 48 2 _depth)
 string(SUBSTRING "${_ihdr}" 50 2 _ctype)
 if(NOT _depth STREQUAL "08" OR NOT _ctype STREQUAL "03")
  message(FATAL_ERROR "LiveArea PNG must be 8-bit indexed (PNG color type 3): ${path}")
 endif()
endfunction()

psv_assert_indexed_png("${CMAKE_SOURCE_DIR}/sce_sys/icon0.png")
psv_assert_indexed_png("${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png")
psv_assert_indexed_png("${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/startup.png")

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(VitaAutoPlugin.vpk ${VITA_TITLEID} eboot.bin
 VERSION ${VITA_VERSION}
 NAME ${VITA_APP_NAME}
 FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
 ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
 ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
 ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
 ${CMAKE_SOURCE_DIR}/assets/icons/home.png assets/icons/home.png
 ${CMAKE_SOURCE_DIR}/assets/icons/plugin.png assets/icons/plugin.png
 ${CMAKE_SOURCE_DIR}/assets/icons/trophy.png assets/icons/trophy.png
 ${CMAKE_SOURCE_DIR}/assets/icons/monitor.png assets/icons/monitor.png
 ${CMAKE_SOURCE_DIR}/assets/icons/overclock.png assets/icons/overclock.png
 ${CMAKE_SOURCE_DIR}/assets/icons/recovery.png assets/icons/recovery.png
 ${CMAKE_SOURCE_DIR}/assets/icons/update.png assets/icons/update.png
 ${CMAKE_SOURCE_DIR}/assets/icons/news.png assets/icons/news.png
 ${CMAKE_SOURCE_DIR}/assets/icons/settings.png assets/icons/settings.png
 ${CMAKE_SOURCE_DIR}/assets/icons/about.png assets/icons/about.png
 ${CMAKE_SOURCE_DIR}/assets/icons/github.png assets/icons/github.png
 ${CMAKE_SOURCE_DIR}/assets/icons/crown.png assets/icons/crown.png
 ${CMAKE_SOURCE_DIR}/assets/icons/save.png assets/icons/save.png
 ${CMAKE_SOURCE_DIR}/assets/icons/backup.png assets/icons/backup.png
 ${CMAKE_SOURCE_DIR}/assets/icons/restore.png assets/icons/restore.png
 ${CMAKE_SOURCE_DIR}/assets/icons/warning.png assets/icons/warning.png
 ${CMAKE_SOURCE_DIR}/assets/icons/check.png assets/icons/check.png
 ${CMAKE_SOURCE_DIR}/assets/icons/status.png assets/icons/status.png
)
