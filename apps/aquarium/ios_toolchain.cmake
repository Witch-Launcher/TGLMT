# ios_toolchain.cmake — CMake toolchain build app iOS (device arm64).
# Dùng: scripts/configure_ios.sh hoặc Makefile (make lib-ios / make xcode-project).
# IOS_DEPLOYMENT_TARGET truyền từ ngoài (-DIOS_DEPLOYMENT_TARGET=15.0) để không
# hardcode version ở nhiều file; toolchain chỉ đọc, không tự đặt.
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_DEPLOYMENT_TARGET "${IOS_DEPLOYMENT_TARGET}")
set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
# -miphoneos-version-min QUAN TRỌNG: thiếu nó binary ghi minos=SDK, iPhone iOS
# thấp hơn từ chối đăng ký app (TrollStore 181).
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0")
endif()
set(_AQUA_IOS_ARCH_FLAGS "-arch arm64 -miphoneos-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
set(CMAKE_C_FLAGS_INIT "${_AQUA_IOS_ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${_AQUA_IOS_ARCH_FLAGS}")
set(CMAKE_OBJCXX_FLAGS_INIT "${_AQUA_IOS_ARCH_FLAGS}")
