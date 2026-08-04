#
# Makefile to build assembler/emulator
#

# Parameters to set:

APP_NAME := bscan

ifeq ($(OS),Windows_NT)
  BIN_DIR    ?= bin
  INT_DIR    ?= int/$(APP_NAME)_windows
  EXECUTABLE ?= $(INT_DIR)/$(APP_NAME).exe
else
  BIN_DIR    ?= bin
  INT_DIR    ?= int/$(APP_NAME)_linux
  EXECUTABLE ?= $(INT_DIR)/$(APP_NAME)
endif

SILENT ?= @
OFLAG  ?=
export OFLAG # export to device makefiles

################################


ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

CC := gcc

CFLAGS := -g $(OFLAG)
# Turning off optimizations for debugging
CFLAGS += -O0
CFLAGS += -I$(ROOT)/include -I$(ROOT)/src
CFLAGS += -Wall -Werror -fshort-wchar -Werror=implicit-function-declaration
CFLAGS += -Wno-multichar
CFLAGS += -Wno-unused-variable -Wno-unused-value -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-result


ifeq ($(OS),Windows_NT)
	# RAYLIB_VER:=raylib-6.0_win64_msvc16
	RAYLIB_VER=raylib-6.0_win64_mingw-w64-msvcrt
    CFLAGS += -I$(ROOT)/libs/$(RAYLIB_VER)/include
    LDFLAGS += $(ROOT)/libs/$(RAYLIB_VER)/lib/libraylib.a -lgdi32 -lwinmm
    LDFLAGS += -lmfplat -lmf -lmfreadwrite -lmfuuid
else
    IS_NIXOS := $(shell test -e /etc/nixos && echo yes)
    
    ifeq ($(IS_NIXOS),yes)
        # Raylib needed for display device (not emulator itself).
        # Devices such as display will be compiled separately into shared libraries
        # in the future meaning emulator won't need raylib dependency.
        # On NixOS we use raylib specified by shell.nix. We also implicitly get include header.
        # This won't work on Ubuntu...
        LDFLAGS += -lraylib
    else
        # @TODO Paths on ubuntu? provide raylib binaries in repo for ubuntu-like installations.
        CFLAGS += -I$(ROOT)/libs/raylib-6.0_linux_amd64/include
        LDFLAGS += $(ROOT)/libs/raylib-6.0_linux_amd64/lib/libraylib.a -lX11
    endif
    
endif



LDFLAGS := -g -lm $(LDFLAGS)


SRC_DIRS := \
	$(ROOT)/src/bscan

rwildcard = $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# SOURCES := $(foreach dir,$(SRC_DIRS),$(call rwildcard,$(dir),*.c) $(call rwildcard,$(dir),*.s))

SRC_FILES := \
	$(foreach dir,$(SRC_DIRS),$(call rwildcard,$(dir),*.c) $(call rwildcard,$(dir),*.s))
 
# Linux
# $(foreach dir,$(SRC_DIRS),$(shell find $(dir) \( -name '*.c' -o -name '*.s' \) ))

OBJ_FILES := $(patsubst $(ROOT)/%.c,$(INT_DIR)/%.o,$(SRC_FILES))
OBJ_FILES := $(patsubst $(ROOT)/%.s,$(INT_DIR)/%.o,$(OBJ_FILES))
DEP_FILES := $(patsubst %.o,%.d,$(OBJ_FILES))

# $(info $(OBJ_FILES) $(SRC_FILES))

ifeq ($(OS),Windows_NT)
	RM_CMD := - cmd /C del /Q
	MKDIR  = - cmd /C mkdir $(subst /,\,$1)
	CP     := - cmd /C copy "$(subst /,\,$1)" "$(subst /,\,$2)"
else
	RM_CMD := rm -rf
	MKDIR  = mkdir -p $1
	CP     := cp $1
endif


all: $(EXECUTABLE) driver

ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean)))
    -include $(DEP_FILES)
endif


$(INT_DIR):
	$(SILENT) $(call MKDIR,$@)

$(INT_DIR)/%.o: $(ROOT)/%.c | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(CC) $(CFLAGS) -c -MD -o $@ $<

$(INT_DIR)/%.o: $(INT_DIR)/%.s | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(AS) $(ASFLAGS) -c -MD $(patsubst %.o,%.d,$@) -o $@ $<

$(EXECUTABLE): $(OBJ_FILES) | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(call MKDIR,$(BIN_DIR))
	$(SILENT) $(CC) -o $@ $^ $(LDFLAGS)
ifeq ($(OS),Windows_NT)
	$(SILENT) cmd /C copy $(subst /,\,$@) $(subst /,\,$(BIN_DIR)/$(notdir $@))
else
	$(SILENT) cp $@ $(BIN_DIR)/$(notdir $@)
endif

.PHONY: clean_all clean driver

driver:
	$(SILENT) $(MAKE) -f $(ROOT)/driver/Makefile

clean:
ifeq ($(OS),Windows_NT)
	$(SILENT) $(RM_CMD) $(subst /,\,$(OBJ_FILES) $(DEP_FILES) $(EXECUTABLE))
else
	$(SILENT) $(RM_CMD) $(OBJ_FILES) $(DEP_FILES) $(EXECUTABLE)
endif
	$(SILENT) $(MAKE) -f $(ROOT)/driver/Makefile clean

clean_all: clean
ifeq ($(OS),Windows_NT)
	$(SILENT) $(RM_CMD) releases
else
	$(SILENT) $(RM_CMD) releases
endif
