# Makefile — Build chuẩn cho TGLMT + AquariumBench.
#
# Mục tiêu: sinh IPA unsigned cài được bằng TrollStore, và FAIL SỚM nếu bất kỳ
# điều kiện nào của LaunchServices chưa đúng (thiếu icon, MinimumOSVersion lệch
# với binary, build nhầm SDK macOS...) — thay vì để bạn cài lên máy rồi mới
# nhận lỗi 181.
#
#   make                = make ipa  (build chuẩn: toolchain Apple qua Xcode)
#   make ipa            IPA unsigned, đã verify — file để cài
#   make ipa-ios15      IPA minos 15.0 (máy iOS 15.x) — file riêng
#   make verify         kiểm tra IPA vừa build
#   make diagnose       khi máy vẫn báo 181: verify + checklist phía device
#   make test           build + ctest trên macOS
#   make help           đầy đủ targets
#
# Biến (ghi đè được):
#   DEPLOY_TARGET=15.0        iOS deployment target (đồng bộ binary + plist)
#   BUNDLE_ID=com.x.y         đổi bundle ID (tránh trùng app đã cài)
#   BUILD_METHOD=xcode|manual xcode = chuẩn (mặc định); manual = link tay bằng clang
#
# Mọi path tuyệt đối từ ROOT nên gọi `make` từ bất kỳ thư mục nào cũng đúng.

ROOT            := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
APP_DIR         := $(ROOT)/apps/aquarium
INC_DIR         := $(ROOT)/include
TOOLCHAIN       := $(APP_DIR)/ios_toolchain.cmake

# --- Cấu hình build iOS ---
DEPLOY_TARGET   ?= 16.0
BUNDLE_ID       ?= com.tglmt.aquarium
DEVICE_FAMILY   ?= 1,2
BUILD_METHOD    ?= xcode
CONFIG          ?= Release

LIB_BUILD       := $(ROOT)/build-iphoneos
LIB_IOS         := $(LIB_BUILD)/libtglmt.a
XCODE_PROJ      := $(APP_DIR)/build-ios/AquariumBench.xcodeproj
XCODE_APP       := $(APP_DIR)/build-ios/$(CONFIG)-iphoneos/AquariumBench.app
ICON_DIR        := $(APP_DIR)/ios_assets
BIN_DIR         := $(ROOT)/build/ios-manual
APP_BIN         := $(BIN_DIR)/AquariumBench

# Tên IPA gắn với deployment target → các bản không ghi đè nhau.
IPA_SUFFIX      := $(if $(filter 16.0,$(DEPLOY_TARGET)),,-$(DEPLOY_TARGET))
IPA             ?= $(APP_DIR)/AquariumBench$(IPA_SUFFIX).ipa
MANUAL_IPA      ?= $(APP_DIR)/AquariumBench-manual$(IPA_SUFFIX).ipa

SDK             := $(shell xcrun --sdk iphoneos --show-sdk-path 2>/dev/null)
SDK_VERSION     := $(shell xcrun --sdk iphoneos --show-sdk-version 2>/dev/null)
VERIFY          ?= scripts/verify_ipa_trollstore.py

CXX_IOS         := xcrun --sdk iphoneos clang++
CXXFLAGS_IOS    := -arch arm64 -miphoneos-version-min=$(DEPLOY_TARGET) -std=c++17 -O2 -fobjc-arc
FRAMEWORKS_IOS  := -framework UIKit -framework Foundation -framework QuartzCore -framework Metal

.PHONY: all ipa ipa-xcode ipa-manual ipa-ios15 app verify check verify-plist \
        help check-tools icons lib-ios lib-mac app-mac test xcode-project xcode-app \
        diagnose clean distclean

all: ipa

help:
	@echo "AquariumBench / TGLMT — targets:"
	@echo ""
	@echo "  Build (dùng hằng ngày):"
	@echo "    make                 = make ipa (khuyên dùng)"
	@echo "    make ipa             IPA unsigned chuẩn (toolchain Apple) + verify → file để cài TrollStore"
	@echo "    make ipa-ios15       IPA minos 15.0 cho máy iOS 15.x (Dopamine) — file riêng"
	@echo "    make verify          Verify lại IPA đã build (không build)"
	@echo "    make diagnose        Verify + checklist khi máy vẫn báo lỗi 181"
	@echo "    make test            Build + chạy unit/integration test trên macOS"
	@echo ""
	@echo "  Thành phần:"
	@echo "    make check-tools     Kiểm tra toolchain (cmake/xcodebuild/Pillow/zip)"
	@echo "    make icons           Sinh icon (thiếu icon = lỗi 181 icon cache)"
	@echo "    make lib-ios         Build libtglmt.a cho iphoneos/arm64"
	@echo "    make lib-mac         Build lib cho macOS (Apple Metal backend)"
	@echo "    make app-mac         Build + chạy benchmark Aquarium trên macOS"
	@echo "    make xcode-project   Sinh Xcode project (mở Xcode để ký + deploy: DEPLOY_IOS.md)"
	@echo "    make xcode-app       Build .app iOS unsigned bằng xcodebuild"
	@echo ""
	@echo "  Dọn dẹp:"
	@echo "    make clean           Xóa artifact build tay + .app của Xcode (giữ IPA)"
	@echo "    make distclean       clean + xóa cache CMake (build-iphoneos, build-ios)"
	@echo ""
	@echo "  Biến:"
	@echo "    DEPLOY_TARGET=$(DEPLOY_TARGET)   iOS min version (đồng bộ binary + Info.plist)"
	@echo "    BUNDLE_ID=$(BUNDLE_ID)"
	@echo "    BUILD_METHOD=$(BUILD_METHOD)     xcode (mặc định, chuẩn) | manual (link tay)"
	@echo ""
	@echo "  Ví dụ:"
	@echo "    make ipa                                   # máy iOS >= 16.0"
	@echo "    make ipa DEPLOY_TARGET=15.0                # máy iOS 15.x"
	@echo "    make ipa BUNDLE_ID=com.tglmt.aquarium2     # tránh trùng app đã cài"

# ------------------------------------------------------------------ toolchain --

check-tools:
	@echo "== check-tools =="
	@command -v cmake >/dev/null || { echo "THIẾU cmake (brew install cmake)"; exit 1; }
	@command -v xcrun >/dev/null || { echo "THIẾU Xcode command line tools (xcode-select --install)"; exit 1; }
	@test -n "$(SDK)" || { echo "THIẾU iPhoneOS SDK — cài Xcode đầy đủ"; exit 1; }
	@command -v zip >/dev/null || { echo "THIẾU zip"; exit 1; }
	@command -v unzip >/dev/null || { echo "THIẾU unzip"; exit 1; }
	@command -v xcodebuild >/dev/null || { echo "THIẾU xcodebuild (cài Xcode đầy đủ)"; exit 1; }
	@python3 -c "import PIL" 2>/dev/null || { echo "THIẾU Pillow: pip3 install pillow"; exit 1; }
	@plutil -lint "$(APP_DIR)/ios_Info.plist" >/dev/null || { echo "ios_Info.plist lỗi cú pháp"; exit 1; }
	@python3 -c "import ast,sys; ast.parse(open('$(ROOT)/$(VERIFY)').read())" \
		|| { echo "verify script lỗi cú pháp"; exit 1; }
	@echo "  iPhoneOS SDK: $(SDK_VERSION)  ($(SDK))"
	@echo "  cmake: $(shell cmake --version | head -n 1)"
	@echo "  DEPLOY_TARGET=$(DEPLOY_TARGET)  BUNDLE_ID=$(BUNDLE_ID)  BUILD_METHOD=$(BUILD_METHOD)"
	@echo "TOOLS OK"

check: check-tools

icons:
	@echo "== icons (TrollStore cần icon, thiếu → 181) =="
	@python3 "$(ROOT)/scripts/gen_ios_icons.py" "$(ICON_DIR)"

# ---------------------------------------------------------------- lib (shared) --

lib-ios:
	@echo "== lib-ios (iphoneos/arm64, minos $(DEPLOY_TARGET)) =="
	@test -n "$(SDK)" || { echo "THIẾU iPhoneOS SDK"; exit 1; }
	@cmake -S "$(ROOT)" -B "$(LIB_BUILD)" -DTGLMT_APPLE_METAL=ON \
		-DCMAKE_TOOLCHAIN_FILE="$(TOOLCHAIN)" \
		-DIOS_DEPLOYMENT_TARGET=$(DEPLOY_TARGET) -DTGLMT_BUILD_TESTS=OFF
	@cmake --build "$(LIB_BUILD)" -j8
	@test -f "$(LIB_IOS)" || { echo "Build lib thất bại: $(LIB_IOS)"; exit 1; }
	@echo "  lib: $(LIB_IOS)"

lib-mac:
	@cmake -S "$(ROOT)" -B "$(ROOT)/build-apple" -DTGLMT_APPLE_METAL=ON
	@cmake --build "$(ROOT)/build-apple" -j8

app-mac: lib-mac
	@sh "$(ROOT)/scripts/build_aquarium_macos.sh"

test:
	@sh "$(ROOT)/scripts/build_and_test.sh"

# ------------------------------------------------------- iOS app chuẩn (Xcode) --

# .app do toolchain Apple sinh ra: Info.plist binary, PkgInfo, DTXcode, icon
# convert chuẩn → LaunchServices chấp nhận nhiều hơn bundle tự dựng tay.
xcode-project: lib-ios
	@sh "$(ROOT)/scripts/configure_ios.sh" "$(DEPLOY_TARGET)" "$(BUNDLE_ID)"

xcode-app: check-tools xcode-project icons
	@echo "== xcode-app (unsigned, toolchain Apple) =="
	@mkdir -p "$(BIN_DIR)"   # phải có TRƯỚC khi tee ghi log, nếu không log sẽ mất
	@xcodebuild -project "$(XCODE_PROJ)" -target AquariumBench \
		-sdk iphoneos -configuration $(CONFIG) -destination 'generic/platform=iOS' \
		IPHONEOS_DEPLOYMENT_TARGET=$(DEPLOY_TARGET) \
		PRODUCT_BUNDLE_IDENTIFIER=$(BUNDLE_ID) \
		CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY="" \
		ONLY_ACTIVE_ARCH=NO > "$(BIN_DIR)/xcodebuild.log" 2>&1; \
	  rc=$$?; \
	  grep -E "warning: (All interface|User-supplied|User supplied)|error:|BUILD (SUCCEEDED|FAILED)" "$(BIN_DIR)/xcodebuild.log" || true; \
	  if [ $$rc -ne 0 ]; then echo "Xcode build thất bại (rc=$$rc) — log: $(BIN_DIR)/xcodebuild.log"; exit 1; fi
	@test -d "$(XCODE_APP)" || { echo "Xcode build không sinh .app — log: $(BIN_DIR)/xcodebuild.log"; exit 1; }
	@test -f "$(XCODE_APP)/Info.plist" || { echo "Thiếu Info.plist trong .app"; exit 1; }
	@echo "  app: $(XCODE_APP)"
	@echo "  log: $(BIN_DIR)/xcodebuild.log"

ipa-xcode: xcode-app
	@mkdir -p "$(BIN_DIR)"
	@echo "== ipa-xcode (minos $(DEPLOY_TARGET)) =="
	@BUNDLE_ID=$(BUNDLE_ID) MAX_MINOS=$(DEPLOY_TARGET) \
		sh "$(ROOT)/scripts/build_ipa_from_app.sh" "$(XCODE_APP)" "$(IPA)"
	@echo ""
	@echo "XONG: file để cài TrollStore →"
	@echo "      $(IPA)"

# ----------------------------------------------- iOS app tay (fallback, link tay) --

# Giữ làm phương án dự phòng: không cần xcodebuild. Nhưng bundle tự dựng dễ thiếu
# chi tiết mà LaunchServices kiểm tra → ưu tiên ipa-xcode.
app-binary: check-tools lib-ios icons
	@echo "== app-binary (clang tay, minos $(DEPLOY_TARGET)) =="
	@mkdir -p "$(BIN_DIR)"
	@$(CXX_IOS) $(CXXFLAGS_IOS) -isysroot "$(SDK)" \
		-I"$(INC_DIR)" -I"$(APP_DIR)" \
		"$(APP_DIR)/aquarium.cpp" "$(APP_DIR)/ios_shell.mm" \
		"$(LIB_IOS)" $(FRAMEWORKS_IOS) -o "$(APP_BIN)"
	@chmod 755 "$(APP_BIN)"
	@lipo -info "$(APP_BIN)"

ipa-manual: app-binary
	@echo "== ipa-manual (đóng gói tay, minos $(DEPLOY_TARGET)) =="
	@BUNDLE_ID=$(BUNDLE_ID) MAX_MINOS=$(DEPLOY_TARGET) \
		sh "$(ROOT)/scripts/build_ipa_trollstore.sh" "$(APP_BIN)" "$(MANUAL_IPA)"
	@echo "XONG (dự phòng): $(MANUAL_IPA)"

# ------------------------------------------------------------------- dispatch --

ipa:
ifeq ($(BUILD_METHOD),manual)
	@$(MAKE) --no-print-directory ipa-manual
else ifeq ($(BUILD_METHOD),xcode)
	@$(MAKE) --no-print-directory ipa-xcode
else
	@echo "BUILD_METHOD không hợp lệ: '$(BUILD_METHOD)' (chọn xcode | manual)"; exit 1
endif

# Bản iOS 15.x — file riêng, không đè bản chính.
ipa-ios15:
	@$(MAKE) --no-print-directory ipa-xcode DEPLOY_TARGET=15.0

# --------------------------------------------------------------------- verify --

verify:
	@echo "== verify $(IPA) =="
	@python3 "$(ROOT)/$(VERIFY)" "$(IPA)" --minos $(DEPLOY_TARGET) --bundle-id $(BUNDLE_ID)

verify-plist:
	@plutil -lint "$(APP_DIR)/ios_Info.plist"

# ------------------------------------------------------------------ diagnose --

diagnose: verify
	@echo ""
	@echo "== diagnose: verify PASS nhưng máy vẫn báo 181 → vấn đề ngoài bundle =="
	@echo "  1. CÁCH LY: cài một IPA khác (app bất kỳ) qua chính TrollStore này."
	@echo "     - App khác cũng 181  → lỗi TrollStore/ldid/device, không phải app này."
	@echo "     - App khác cài được → gửi lại log để đối chiếu bundle."
	@echo "  2. TrollStore → Settings: đổi Installation Method custom ↔ installd."
	@echo "  3. Gỡ app cùng bundle ID ($(BUNDLE_ID)) nếu có, reboot rồi cài lại."
	@echo "  4. TrollStore → Settings → Refresh Icon Cache; cập nhật ldid mới nhất."
	@echo "  5. Đổi bundle ID để loại trừ xung đột: make ipa BUNDLE_ID=com.tglmt.aqu2"
	@echo "  6. Kiểm tra log: plutil -p '$(IPA)' | grep -E 'MinimumOSVersion|CFBundleIdentifier'"
	@echo ""
	@echo "-- Thông tin build hiện tại --"
	@echo "  DEPLOY_TARGET=$(DEPLOY_TARGET)  BUILD_METHOD=$(BUILD_METHOD)  BUNDLE_ID=$(BUNDLE_ID)"
	@echo "  SDK=$(SDK_VERSION)"
	@test -f "$(XCODE_APP)/$(notdir $(APP_BIN))" && \
		otool -l "$(XCODE_APP)/AquariumBench" | grep -E "platform|minos|sdk " | head -n 3 || true

# --------------------------------------------------------------------- clean --

clean:
	@rm -rf "$(BIN_DIR)" "$(APP_DIR)/build-ios/$(CONFIG)-iphoneos" "$(APP_DIR)/build-ios/build"
	@echo "cleaned: build tay + .app của Xcode (giữ IPA + cache CMake)"

distclean: clean
	@rm -rf "$(LIB_BUILD)" "$(APP_DIR)/build-ios"
	@echo "distcleaned: xóa cả cache CMake (giữ IPA + sources)"
