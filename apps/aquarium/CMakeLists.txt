cmake_minimum_required(VERSION 3.20)
# Aquarium benchmark — STANDALONE project, chỉ link TGLMT ĐÃ BUILD (lib + headers).
# Không bao giờ biên dịch lại sources TGLMT ở đây (chứng minh dịch qua artifact).
#   macOS: cmake -S . -B build -DTGLMT_INCLUDE_DIR=... -DTGLMT_LIB=... && cmake --build build
#   iOS:   scripts/configure_ios.sh (ra Xcode project, mở bằng Xcode để ký + deploy)
project(AquariumBench VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Tham số app iOS (Makefile truyền vào, không hardcode ở nhiều nơi) ---
# Trước đây IPHONEOS_DEPLOYMENT_TARGET hardcode "16.0" trong khi ios_Info.plist
# cũng ghi MinimumOSVersion 16.0 → hạ version không được, dễ dính lỗi TrollStore 181.
set(AQUA_BUNDLE_ID "com.tglmt.aquarium" CACHE STRING "Bundle ID app iOS")
set(AQUA_DEVICE_FAMILY "1,2" CACHE STRING "TARGETED_DEVICE_FAMILY (iPhone,iPad)")
set(AQUA_IOS_MIN_VERSION "16.0" CACHE STRING "IOS_DEPLOYMENT_TARGET cho app iOS")

if(NOT TGLMT_INCLUDE_DIR OR NOT TGLMT_LIB)
  message(FATAL_ERROR "Thiếu -DTGLMT_INCLUDE_DIR=... -DTGLMT_LIB=... (file TGLMT đã build). "
    "VD: -DTGLMT_INCLUDE_DIR=/Volumes/D/TGLMT/include "
    "-DTGLMT_LIB=/Volumes/D/TGLMT/build-apple/libtglmt.a")
endif()
if(NOT EXISTS "${TGLMT_LIB}")
  message(FATAL_ERROR "Không thấy lib TGLMT đã build: ${TGLMT_LIB}")
endif()
if(NOT EXISTS "${TGLMT_INCLUDE_DIR}/tglmt/gl46.h")
  message(FATAL_ERROR " Headers TGLMT không hợp lệ: ${TGLMT_INCLUDE_DIR}")
endif()
message(STATUS "TGLMT include: ${TGLMT_INCLUDE_DIR}")
message(STATUS "TGLMT lib (built artifact): ${TGLMT_LIB}")

add_library(aquarium_core STATIC aquarium.cpp)
target_include_directories(aquarium_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${TGLMT_INCLUDE_DIR})

if(APPLE AND NOT IOS)
  # ---- macOS app: shell AppKit + CAMetalLayer ----
  enable_language(OBJCXX)
  add_executable(AquariumBench MACOSX_BUNDLE macos_shell.mm)
  set_source_files_properties(macos_shell.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  target_include_directories(AquariumBench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${TGLMT_INCLUDE_DIR})
  target_link_libraries(AquariumBench PRIVATE aquarium_core ${TGLMT_LIB}
    "-framework Cocoa" "-framework QuartzCore" "-framework Metal")
  set_target_properties(AquariumBench PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/macos_Info.plist)
elseif(IOS)
  # ---- iOS app: shell UIKit ----
  enable_language(OBJCXX)
  add_executable(AquariumBench MACOSX_BUNDLE ios_shell.mm)
  set_source_files_properties(ios_shell.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  target_include_directories(AquariumBench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${TGLMT_INCLUDE_DIR})
  target_link_libraries(AquariumBench PRIVATE aquarium_core ${TGLMT_LIB}
    "-framework UIKit" "-framework Foundation" "-framework QuartzCore" "-framework Metal")
  set_target_properties(AquariumBench PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "${AQUA_BUNDLE_ID}"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios_Info.plist
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "${AQUA_DEVICE_FAMILY}"
    # PRODUCT_BUNDLE_IDENTIFIER: nếu thiếu, Xcode cảnh báo "must be the same as
    # the PRODUCT_BUNDLE_IDENTIFIER build setting value ''" → bundle không sạch.
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${AQUA_BUNDLE_ID}"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${AQUA_IOS_MIN_VERSION}")
  # Shaders vào bundle để app đọc lúc chạy (vẫn là GLSL thuần — dịch runtime qua TGLMT)
  # iOS bundle phẳng (không có Contents/): đặt ở "shaders" để khớp
  # pathForResource:inDirectory:@"shaders" trong ios_shell.mm.
  # TrollStore 181 = "Failed to add app to icon cache" khi thiếu icon → kèm icon ở bundle root.
  target_sources(AquariumBench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fish.vert)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/shaders/fish.vert PROPERTIES
    MACOSX_PACKAGE_LOCATION shaders)
  foreach(S fish.frag water.vert water.frag sand.vert sand.frag plant.vert plant.frag
            bubble.vert bubble.frag bg.vert bg.frag hud.vert hud.frag)
    target_sources(AquariumBench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${S})
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/shaders/${S} PROPERTIES
      MACOSX_PACKAGE_LOCATION shaders)
  endforeach()
  # Icons: phải nằm ở bundle root (không set subdir) để icon cache tìm thấy.
  # Liệt kê rõ để Xcode copy vào bundle; script TrollStore cũng copy tay.
  file(GLOB AQUA_ICONS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ios_assets/*.png)
  if(AQUA_ICONS)
    target_sources(AquariumBench PRIVATE ${AQUA_ICONS})
    set_source_files_properties(${AQUA_ICONS} PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources)
  endif()
endif()
