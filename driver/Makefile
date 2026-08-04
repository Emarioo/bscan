#
# Makefile to build assembler/emulator
#

# Parameters to set:

APP_NAME := driver_bscan

ifeq ($(OS),Windows_NT)
  BIN_DIR    ?= bin
  INT_DIR    ?= int/$(APP_NAME)_windows
  EXECUTABLE ?= $(INT_DIR)/$(APP_NAME).dll
else
  BIN_DIR    ?= bin
  INT_DIR    ?= int/$(APP_NAME)_linux
  EXECUTABLE ?= $(INT_DIR)/lib$(APP_NAME).so
endif

SILENT ?= 
OFLAG  ?=
export OFLAG # export to device makefiles


################################

ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
OPENVR_ROOT ?= $(abspath $(ROOT)/../../VENDOR/openvr)

ifeq ($(OS),Windows_NT)

# Flags to be inspired by
# CL.exe /c /I..\..\..\..\headers /IE:\dev\VENDOR\openvr\samples\drivers\/utils/driverlog /IE:\dev\VENDOR\openvr\samples\drivers\/utils/vrmath /ZI /JMC /nologo /W3 /WX- /diagnostics:column /sdl /Od /D _DEBUG /D _CONSOLE /D _WINDLL /D _UNICODE /D UNICODE /Gm- /EHsc /RTC1 /MDd /GS /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /permissive- /Fo"x64\Debug\\" /Fd"x64\Debug\vc143.pdb" /external:W3 /Gd /TP /FC /errorReport:prompt src\tracker_device_driver.cpp src\device_provider.cpp src\hmd_driver_factory.cpp
# link.exe /ERRORREPORT:PROMPT /OUT:"E:\dev\VENDOR\openvr\samples\drivers\output\drivers\simpletrackers\bin\win64\driver_simpletrackers.dll" /INCREMENTAL /ILK:"x64\Debug\driver_simpletrackers.ilk" /NOLOGO /LIBPATH:..\..\..\..\lib\win64 openvr_api.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed /DEBUG /PDB:"E:\dev\VENDOR\openvr\samples\drivers\output\drivers\simpletrackers\bin\win64\driver_simpletrackers.pdb" /SUBSYSTEM:CONSOLE /TLBID:1 /DYNAMICBASE /NXCOMPAT /IMPLIB:"E:\dev\VENDOR\openvr\samples\drivers\output\drivers\simpletrackers\bin\win64\driver_simpletrackers.lib" /MACHINE:X64 /DLL x64\Debug\tracker_device_driver.obj

	CC := cl
	CPP := cl
	LD := link

	CFLAGS := -nologo -DEBUG $(OFLAG)
	# Turning off optimizations for debugging
	# CFLAGS += -O0
	CFLAGS += -I$(ROOT)/include -I$(ROOT)/src -I$(ROOT)/../src -I$(OPENVR_ROOT)/headers
	CPPFLAGS := $(CFLAGS)

	LDFLAGS := -nologo -DLL -DEBUG -MACHINE:X64 -LIBPATH:$(OPENVR_ROOT)/lib/win64 openvr_api.lib
else
	CC := gcc
	CPP := g++

	CFLAGS := -g $(OFLAG)
	# Turning off optimizations for debugging
	CFLAGS += -O0
	CFLAGS += -I$(ROOT)/include -I$(ROOT)/src
	CFLAGS += -I$(OPENVR_ROOT)/headers
	CFLAGS += -Wall -Werror -fshort-wchar -fPIC
	CFLAGS += -Wno-multichar
	CFLAGS += -Wno-unused-variable -Wno-unused-value -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-result

	CPPFLAGS := $(CFLAGS)

	# C specific flags
	CFLAGS += -Werror=implicit-function-declaration

    IS_NIXOS := $(shell test -e /etc/nixos && echo yes)
    
    ifeq ($(IS_NIXOS),yes)
        # Raylib needed for display device (not emulator itself).
        # Devices such as display will be compiled separately into shared libraries
        # in the future meaning emulator won't need raylib dependency.
        # On NixOS we use raylib specified by shell.nix. We also implicitly get include header.
        # This won't work on Ubuntu...
        # LDFLAGS += -lraylib
    else
        # @TODO Paths on ubuntu? provide raylib binaries in repo for ubuntu-like installations.
    endif
	LDFLAGS += $(OPENVR_ROOT)/lib/linux64/libopenvr_api.so
	LDFLAGS := -shared -g -lm $(LDFLAGS)
endif





SRC_DIRS := \
	$(ROOT)/src \
	$(ROOT)/../src/bscan/common

rwildcard = $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# SOURCES := $(foreach dir,$(SRC_DIRS),$(call rwildcard,$(dir),*.c) $(call rwildcard,$(dir),*.s))

SRC_FILES := \
	$(foreach dir,$(SRC_DIRS),$(call rwildcard,$(dir),*.cpp) $(call rwildcard,$(dir),*.c) $(call rwildcard,$(dir),*.s))
 
# Linux
# $(foreach dir,$(SRC_DIRS),$(shell find $(dir) \( -name '*.c' -o -name '*.s' \) ))

OBJ_FILES := $(patsubst $(ROOT)/%.c,$(INT_DIR)/%.o,$(SRC_FILES))
OBJ_FILES := $(patsubst $(ROOT)/%.cpp,$(INT_DIR)/%.o,$(OBJ_FILES))
OBJ_FILES := $(patsubst $(ROOT)/%.s,$(INT_DIR)/%.o,$(OBJ_FILES))
DEP_FILES := $(patsubst %.o,%.d,$(OBJ_FILES))

# $(info $(OBJ_FILES) $(SRC_FILES))

ifeq ($(OS),Windows_NT)
	RM_CMD := - cmd /C del /Q
	MKDIR  = - cmd /C mkdir $(subst /,\,$1)
	CP     := - cmd /C copy "$(subst /,\,$1)" "$(subst /,\,$2)"
else
	RM_CMD := rm -f
	MKDIR  = mkdir -p $1
	CP     := cp $1
endif


all: $(EXECUTABLE)

ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean)))
    -include $(DEP_FILES)
endif

ifeq ($(OS),Windows_NT)


$(INT_DIR):
	$(SILENT) $(call MKDIR,$@)

$(INT_DIR)/%.o: $(ROOT)/%.cpp | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(CPP) $(CPPFLAGS) -c -MD -Fo:$@ $<

$(INT_DIR)/%.o: $(ROOT)/%.c | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(CC) $(CFLAGS) -c -MD -Fo:$@ $<

$(EXECUTABLE): $(OBJ_FILES) | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(call MKDIR,$(BIN_DIR))
	$(SILENT) $(LD) -OUT:$@ $^ $(LDFLAGS)
	$(SILENT) cmd /C copy $(subst /,\,$@) $(subst /,\,$(BIN_DIR)/$(notdir $@))

else


$(INT_DIR):
	$(SILENT) $(call MKDIR,$@)

$(INT_DIR)/%.o: $(ROOT)/%.cpp | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(CPP) $(CPPFLAGS) -c -MD -o $@ $<

$(INT_DIR)/%.o: $(ROOT)/%.c | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(CC) $(CFLAGS) -c -MD -o $@ $<

$(INT_DIR)/%.o: $(INT_DIR)/%.s | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(AS) $(ASFLAGS) -c -MD $(patsubst %.o,%.d,$@) -o $@ $<

$(EXECUTABLE): $(OBJ_FILES) | $(INT_DIR)
	$(SILENT) $(call MKDIR,$(@D))
	$(SILENT) $(call MKDIR,$(BIN_DIR))
	$(SILENT) $(CPP) -o $@ $^ $(LDFLAGS)
	$(SILENT) cp $@ $(BIN_DIR)/$(notdir $@)

endif


.PHONY: clean


clean:
ifeq ($(OS),Windows_NT)
	$(SILENT) $(RM_CMD) $(subst /,\,$(OBJ_FILES) $(DEP_FILES) $(EXECUTABLE))
else
	$(SILENT) $(RM_CMD) $(OBJ_FILES) $(DEP_FILES) $(EXECUTABLE)
endif

