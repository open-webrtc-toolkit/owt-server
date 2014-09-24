# This file is generated by gyp; do not edit.

TOOLSET := host
TARGET := genversion
DEFS_Default := 

# Flags passed to all source files.
CFLAGS_Default := -std=gnu99

# Flags passed to only C files.
CFLAGS_C_Default := 

# Flags passed to only C++ files.
CFLAGS_CC_Default := 

INCS_Default := -Ithird_party/yasm/source/config/linux \
	-Ithird_party/yasm/source/patched-yasm

OBJS := $(obj).host/$(TARGET)/third_party/yasm/source/patched-yasm/modules/preprocs/nasm/genversion.o

# Add to the list of files we specially track dependencies for.
all_deps += $(OBJS)

# Make sure our dependencies are built before any of us.
$(OBJS): | $(obj).host/third_party/yasm/config_sources.stamp

# CFLAGS et al overrides must be target-local.
# See "Target-specific Variable Values" in the GNU Make manual.
$(OBJS): TOOLSET := $(TOOLSET)
$(OBJS): GYP_CFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE)) $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_C_$(BUILDTYPE))
$(OBJS): GYP_CXXFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE)) $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_CC_$(BUILDTYPE))

# Suffix rules, putting all outputs into $(obj).

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(srcdir)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

# Try building from generated source, too.

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj).$(TOOLSET)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

# End of this set of suffix rules
### Rules for final target.
LDFLAGS_Default := 

LIBS := 

$(builddir)/genversion: GYP_LDFLAGS := $(LDFLAGS_$(BUILDTYPE))
$(builddir)/genversion: LIBS := $(LIBS)
$(builddir)/genversion: LD_INPUTS := $(OBJS)
$(builddir)/genversion: TOOLSET := $(TOOLSET)
$(builddir)/genversion: $(OBJS) FORCE_DO_CMD
	$(call do_cmd,link)

all_deps += $(builddir)/genversion
# Add target alias
.PHONY: genversion
genversion: $(builddir)/genversion

# Add executable to "all" target.
.PHONY: all
all: $(builddir)/genversion

