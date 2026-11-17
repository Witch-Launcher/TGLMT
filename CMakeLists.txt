cmake_minimum_required(VERSION 3.20)
project(TGLMT VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TGLMT_APPLE_METAL "Biên dịch AppleMetalBridge.mm (chỉ macOS/iOS)" OFF)
option(TGLMT_BUILD_TESTS "Build tests" ON)
option(TGLMT_BUILD_SHARED "Build tglmt.dylib shared cho launcher render" ON)
option(TGLMT_FORCE_DIAG "Chẩn đoán luôn-on (máy thật; TGLMT_DIAG=1 không set được từ launcher)" OFF)
if(TGLMT_FORCE_DIAG)
  add_compile_definitions(TGLMT_FORCE_DIAG=1)
endif()

include_directories(include)

# Linker nhanh: lld (LLVM) link nhanh hơn ld64 ~40%. Có sẵn trên máy dev thì
# dùng, không có thì fallback ld64 (build vẫn đúng, chỉ chậm hơn).
find_program(TGLMT_LLD ld.lld)
if(TGLMT_LLD)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
endif()

# Số bản dựng TGLMT — nguồn duy nhất cho glGetString(GL_VERSION) (hiện trên F3
# qua GpuDevice.getVersion) và dòng diag [TGLMT] build=. Lấy từ git lúc
# configure; commit mới tự kích hoạt configure lại nhờ CONFIGURE_DEPENDS trên
# .git/logs/HEAD (mất git → "dev").
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS .git/logs/HEAD)
execute_process(COMMAND git rev-list --count HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE TGLMT_BUILD_NUM OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE TGLMT_BUILD_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git status --porcelain --untracked-files=no
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE TGLMT_BUILD_DIRTY OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(TGLMT_BUILD_NUM MATCHES "^[0-9]+$" AND TGLMT_BUILD_HASH)
  set(TGLMT_BUILD_STRING "${TGLMT_BUILD_NUM}.${TGLMT_BUILD_HASH}")
  if(TGLMT_BUILD_DIRTY)
    string(APPEND TGLMT_BUILD_STRING "+dirty")
  endif()
else()
  set(TGLMT_BUILD_STRING "dev")
endif()
configure_file(cmake/tglmt_buildinfo.h.in ${CMAKE_BINARY_DIR}/generated/tglmt_buildinfo.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/generated)

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

# Object library: nguồn cho CẢ static lẫn shared. Trước đây add_library cho
# từng target với cùng danh sách source → MỖI file .cpp biên dịch HAI LẦN
# (một cho tglmt static, một cho tglmt_shared) → build sạch tốn ~2× thời gian.
# Giờ biên dịch 1 lần, link ra 2 target.
add_library(tglmt_objs OBJECT ${TGLMT_CORE} ${TGLMT_GL} ${TGLMT_METAL})
target_include_directories(tglmt_objs PUBLIC include)
set_target_properties(tglmt_objs PROPERTIES POSITION_INDEPENDENT_CODE ON)

# glsl_to_msl.cpp biên dịch -O3 mất ~29s (file này có MỘT hàm ConvertGLSLtoMSL
# ~4200 dòng → LLVM CodeGen + optimizer superlinear theo kích thước hàm).
# Converter CHỈ chạy lúc compile/link shader (không phải mỗi frame) nên hạ
# optim xuống -O1 không tốn runtime: đo thực tế 0.71ms/convert (-O3) vs
# 0.74ms (-O1) trên shader nặng nhất → +4% ở thời điểm load, đổi lại build
# nhanh hơn ~6s.
set_source_files_properties(src/gl/glsl_to_msl.cpp PROPERTIES COMPILE_OPTIONS "-O1")

add_library(tglmt STATIC $<TARGET_OBJECTS:tglmt_objs>)
target_include_directories(tglmt PUBLIC include)

if(APPLE AND TGLMT_APPLE_METAL)
  target_link_libraries(tglmt PUBLIC "-framework Metal" "-framework Foundation" "-framework QuartzCore")
endif()

# Shared dylib cho launcher (dlopen/dlsym lwjgl): cùng sources với static.
# Output: libtglmt.dylib (macOS) / libtglmt.dylib arm64 (iOS device).
if(TGLMT_BUILD_SHARED)
  add_library(tglmt_shared SHARED $<TARGET_OBJECTS:tglmt_objs>)
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
  file(GLOB TGLMT_TESTS CONFIGURE_DEPENDS
       tests/unit/*.cpp tests/integration/*.cpp tests/regression/*.cpp
       tests/api/*.cpp tests/conformance/*.cpp tests/shaders/*.cpp)
  # PCH DÙNG CHUNG cho mọi test: 140/167 file đều include "tglmt/gl46.h" +
  # "tglmt/Context.h" (Context.h kéo theo MetalInterface.h + GLSLConverter.h).
  # Không có PCH thì mỗi test parse lại ~4k dòng header → ~1.2s/test →
  # ~200s CPU cho 167 test.
  # Một PCH cho TẤT CẢ test (REUSE_FROM) — không phải PCH riêng cho từng
  # target: 167 bản PCH (~30MB mỗi bản) sẽ đầy đĩa.
  add_library(tglmt_test_pch OBJECT EXCLUDE_FROM_ALL tests/pch_stub.cpp)
  target_precompile_headers(tglmt_test_pch PRIVATE
    <tglmt/gl46.h> <tglmt/Context.h>)
  # Link test với OBJECT library `tglmt_objs` (không phải archive tĩnh
  # `tglmt`) — lld sẽ LOẠI các object chỉ chứa static-init registration của
  # các built-in pass (src/gl/glsl_builtins_*.cpp) vì không có symbol nào được
  # tham chiếu ⇒ mọi pass là NO-OP trong binary test ⇒ test sai (đỏ oan / xanh
  # giả). `tglmt_objs` là OBJECT library nên mọi object đều được đưa vào link.
  foreach(T ${TGLMT_TESTS})
    get_filename_component(NAME ${T} NAME_WE)
    add_executable(${NAME} ${T})
    target_link_libraries(${NAME} PRIVATE tglmt_objs)
    if(APPLE AND TGLMT_APPLE_METAL)
      target_link_libraries(${NAME} PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")
    endif()
    target_precompile_headers(${NAME} REUSE_FROM tglmt_test_pch)
    target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    target_compile_definitions(${NAME} PRIVATE
      TGLMT_CORPUS_DIR="${CMAKE_SOURCE_DIR}/tests/corpus/mc-26.1.2-shaders"
      TGLMT_ROOT="${CMAKE_SOURCE_DIR}")
    # WORKING_DIRECTORY = source dir: 10 test dùng đường dẫn tương đối tới
    # fixture (shaders/*.metal, apps/aquarium/shaders/...) nên khi chạy từ
    # build dir chúng tự SKIP — ctest vẫn báo PASS ⇒ mất ngay các test pixel
    # mạnh nhất của repo (blend, msaa, raster, rop, spirv chain, tess...).
    add_test(NAME ${NAME} COMMAND ${NAME} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  endforeach()

  # Diagnostic runtime probe for known static gaps (not a CTest gate).
  add_executable(truth_probe tests/truth/probe_gaps.cpp)
  target_link_libraries(truth_probe PRIVATE tglmt)

  # --- Benchmarks -------------------------------------------------------
  # Đo thời gian biên dịch shader + hiệu năng runtime. KHÔNG phải CTest gate
  # (số đo phụ thuộc máy) — chạy tay và lưu JSON vào benchmarks/<phase>/.
  file(GLOB TGLMT_BENCH CONFIGURE_DEPENDS
       benchmarks/compilation/*.cpp benchmarks/compilation/*.mm
       benchmarks/runtime/*.cpp)
  foreach(B ${TGLMT_BENCH})
    get_filename_component(BN ${B} NAME_WE)
    add_executable(${BN} ${B})
    target_link_libraries(${BN} PRIVATE tglmt)
    target_include_directories(${BN} PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    if(B MATCHES "\\.mm$")
      # bench_upload.mm đo API Metal thô (đường shared storage = iOS/A11).
      enable_language(OBJCXX)
      set_source_files_properties(${B} PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc" LANGUAGE OBJCXX)
      if(APPLE)
        target_link_libraries(${BN} PRIVATE "-framework Metal" "-framework Foundation")
      endif()
    endif()
  endforeach()
endif()
