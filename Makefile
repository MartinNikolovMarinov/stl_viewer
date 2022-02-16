# OSSFPTI is an acronym for OS_SPECIFIC_SRC_FILES_PREFIX_TO_IGNORE
ifeq ($(OS),Windows_NT)
	OS_DETECTED = windows
    OSSFPTI     = unix
    BIN_NAME    = main.exe
else
	OS_DETECTED = unix
    OSSFPTI     = win64
    BIN_NAME    = main
endif

CC              = g++
COMMON_CFLAGS   = -ggdb -std=c++17 -Wno-unknown-pragmas
CFLAGS          = $(COMMON_CFLAGS) -O0 -g -Wall -Wno-unused
PROD_CFLAGS     = $(COMMON_CFLAGS) -O3
DEBUG_DEFINES   = -DDEBUG=1 -DUSE_ASSERT=1
PROD_DEFINES    = -DDEBUG=0 -DUSE_ASSERT=1
SRC             = main.cpp $(shell  find ./src -type f \( -iname "*.cpp" \) -print)
CORE            = $(shell find ./lib/core/src -type f \( -iname "*.cpp" -and -not -path "*/test/*" -and -not -iname "$(OSSFPTI)*" \) -print)
OUT_DIR         = build

.PHONY: help
help:
	@echo Supported targets:
	@cat $(MAKEFILE_LIST) | grep -e "^[\.a-zA-Z0-9_-]*: *.*## *" | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-35s\033[0m %s\n", $$1, $$2}'

.DEFAULT_GOAL := help

.PHONY: build
build: clean ## Build the project in the build folder. Creates ./build folder if it does not exist.
	mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $(DEBUG_DEFINES) -o $(OUT_DIR)/$(BIN_NAME) $(SRC) $(CORE)

.PHONY: build_prod
build_prod: clean ## Same as build, but optimizes aggresivly and generates no debug information.
	mkdir -p $(OUT_DIR)
	$(CC) $(PROD_CFLAGS) $(PROD_DEFINES) $(PROD_ENV) -o $(OUT_DIR)/$(BIN_NAME) $(SRC) $(CORE)

.PHONY: clean
clean: ## Deletes the build folder.
	rm -rf $(OUT_DIR)