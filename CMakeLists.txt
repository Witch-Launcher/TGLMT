cmake_minimum_required(VERSION 3.20)
project(TGLMT VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TGLMT_APPLE_METAL "Biên dịch AppleMetalBridge.mm (chỉ macOS/iOS)" OFF)
option(TGLMT_BUILD_TESTS "Build tests" ON)
option(TGLMT_BUILD_SHARED "Build tglmt.dylib shared cho launcher render" ON)

include_directories(include)

file(GLOB TGLMT_CORE CONFIGURE_DEPENDS src/core/*.cpp)
file(GLOB TGLMT_GL CONFIGURE_DEPENDS src/gl/*.cpp)
list(FILTER TGLMT_GL EXCLUDE REGEX ".*AppleMetalBridge.*")

set(TGLMT_METAL src/metal/NullMetalBackend.cpp)
if(TGLMT_APPLE_METAL AND APPLE)
  enable_language(OBJCXX)
  list(APPEND TGLMT_METAL src/metal/AppleMetalBridge.mm)
  add_compile_definitions(TGLMT_APPLE_METAL=1)
  # ARC cho file ObjC++ cầu (quản lý id<MTL*> an toàn)
  set_source_files_properties(src/metal/AppleMetalBridge.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
endif()

add_library(tglmt STATIC ${TGLMT_CORE} ${TGLMT_GL} ${TGLMT_METAL})
target_include_directories(tglmt PUBLIC include)

if(APPLE AND TGLMT_APPLE_METAL)
  target_link_libraries(tglmt PUBLIC "-framework Metal" "-framework Foundation" "-framework QuartzCore")
endif()

# Shared dylib cho launcher (dlopen/dlsym lwjgl): cùng sources với static.
# Output: libtglmt.dylib (macOS) / libtglmt.dylib arm64 (iOS device).
if(TGLMT_BUILD_SHARED)
  add_library(tglmt_shared SHARED ${TGLMT_CORE} ${TGLMT_GL} ${TGLMT_METAL})
  target_include_directories(tglmt_shared PUBLIC include)
  set_target_properties(tglmt_shared PROPERTIES
    OUTPUT_NAME tglmt
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
    BUILD_WITH_INSTALL_RPATH TRUE
  )
  if(APPLE AND TGLMT_APPLE_METAL)
    target_link_libraries(tglmt_shared PUBLIC "-framework Metal" "-framework Foundation" "-framework QuartzCore")
  endif()
endif()

if(TGLMT_BUILD_TESTS)
  enable_testing()
  file(GLOB TGLMT_TESTS CONFIGURE_DEPENDS tests/unit/*.cpp tests/integration/*.cpp)
  foreach(T ${TGLMT_TESTS})
    get_filename_component(NAME ${T} NAME_WE)
    add_executable(${NAME} ${T})
    target_link_libraries(${NAME} PRIVATE tglmt)
    add_test(NAME ${NAME} COMMAND ${NAME})
  endforeach()
endif()
