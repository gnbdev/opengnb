REPO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
UNITY_DIR := $(REPO_ROOT)/libs/Unity/src
UNITY_CHECKSUM_DIR := $(REPO_ROOT)/libs/Unity
UNITY_CHECKSUMS := $(UNITY_CHECKSUM_DIR)/SHA256SUMS
BUILD_DIR := $(REPO_ROOT)/build/tests/$(PLATFORM)

ifeq ($(PLATFORM),linux)
EXECUTABLE_SUFFIX :=
else ifeq ($(PLATFORM),mingw)
EXECUTABLE_SUFFIX := .exe
else
$(error PLATFORM 必须显式设置为 linux 或 mingw)
endif

RUNNER := $(BUILD_DIR)/unity_empty_runner$(EXECUTABLE_SUFFIX)
SOURCES := $(UNITY_DIR)/unity.c $(REPO_ROOT)/tests/framework/unity_empty_runner.c
HEADERS := $(UNITY_DIR)/unity.h $(UNITY_DIR)/unity_internals.h

CPPFLAGS += -I$(UNITY_DIR)
CFLAGS += -std=c99 -Wall -Wextra -Werror

.PHONY: test-framework verify-unity test-clean

test-framework: verify-unity $(RUNNER)
	@$(RUNNER)

verify-unity:
	cd $(UNITY_CHECKSUM_DIR) && sha256sum --check $(notdir $(UNITY_CHECKSUMS))

$(RUNNER): $(SOURCES) $(HEADERS) $(MAKEFILE_PATH) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $@

test-clean:
	$(RM) -r $(BUILD_DIR)
