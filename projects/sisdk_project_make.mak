.SUFFIXES:				# ignore builtin rules
.PHONY: clean configure build clean_build configure_build

TYPE ?= debug
PRJ_MACROS ?= ""

ifndef CMAKELIST_DIR
$(error CMAKELIST_DIR is not set)
endif
BUILD_DIR := $(CMAKELIST_DIR)/build

clean_build: clean configure build
configure_build: configure build
clean:
	@echo 'Cleaning build directory!'
	cmake -E remove_directory $(BUILD_DIR)
configure:
	@echo 'CMAKE $(TYPE) configure...!'
	@cmake $(CMAKELIST_DIR) --preset $(TYPE) -DCMAKE_C_FLAGS="$(CMAKE_C_FLAGS) $(PRJ_MACROS)"
build:
	@echo 'CMAKE build...!'
	cmake --build $(BUILD_DIR) --parallel
