BUILD_DIR := build
ANALYZE_DIR := build-analyze

UNAME_S := $(shell uname -s)

VCPKG_TARGET_TRIPLET ?=
VCPKG_HOST_TRIPLET ?=
CMAKE_OSX_ARCHITECTURES ?=

ifeq ($(UNAME_S),Darwin)
VCPKG_TARGET_TRIPLET ?= arm64-osx
VCPKG_HOST_TRIPLET ?= arm64-osx
CMAKE_OSX_ARCHITECTURES ?= arm64
endif

CMAKE_FLAGS := -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE=$(CURDIR)/vcpkg/scripts/buildsystems/vcpkg.cmake \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ifneq ($(strip $(VCPKG_TARGET_TRIPLET)),)
CMAKE_FLAGS += -DVCPKG_TARGET_TRIPLET=$(VCPKG_TARGET_TRIPLET)
endif
ifneq ($(strip $(VCPKG_HOST_TRIPLET)),)
CMAKE_FLAGS += -DVCPKG_HOST_TRIPLET=$(VCPKG_HOST_TRIPLET)
endif
ifneq ($(strip $(CMAKE_OSX_ARCHITECTURES)),)
CMAKE_FLAGS += -DCMAKE_OSX_ARCHITECTURES=$(CMAKE_OSX_ARCHITECTURES)
endif

.PHONY: configure configure-analyze analyze tidy format format-check build run clean clean-analyze

configure:
	VCPKG_DISABLE_METRICS=1 \
	VCPKG_DEFAULT_TRIPLET=$(VCPKG_TARGET_TRIPLET) \
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build: configure
	cmake --build $(BUILD_DIR)

run: build 
	./$(BUILD_DIR)/src/engine_app

configure-analyze:
	VCPKG_DISABLE_METRICS=1 \
	VCPKG_DEFAULT_TRIPLET=$(VCPKG_TARGET_TRIPLET) \
	scan-build --status-bugs cmake -S . -B $(ANALYZE_DIR) $(CMAKE_FLAGS)

analyze: configure-analyze
	scan-build --status-bugs cmake --build $(ANALYZE_DIR)

tidy: configure
	clang-tidy -p $(BUILD_DIR) src/**/*.cpp

format:
	find . \( -name '*.cpp' -o -name '*.cc' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' -o -name '*.inl' \) \
	  -not -path './build*/*' -not -path './_deps/*' -not -path './vcpkg/*' \
	  -print0 | xargs -0 clang-format -i

format-check:
	find . \( -name '*.cpp' -o -name '*.cc' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' -o -name '*.inl' \) \
	  -not -path './build*/*' -not -path './_deps/*' -not -path './vcpkg/*' \
	  -print0 | xargs -0 clang-format -n -Werror

clean:
	rm -rf $(BUILD_DIR)

clean-analyze:
	rm -rf $(ANALYZE_DIR)
