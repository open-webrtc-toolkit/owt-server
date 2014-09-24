# This file is generated by gyp; do not edit.

TOOLSET := host
TARGET := generate_files
### Rules for action "generate_x86_insn":
quiet_cmd_generate_files_generate_x86_insn = ACTION Running source/patched-yasm/modules/arch/x86/gen_x86_insn.py. $@
cmd_generate_files_generate_x86_insn = export LD_LIBRARY_PATH=$(builddir)/lib.host:$(builddir)/lib.target:$$LD_LIBRARY_PATH; cd third_party/yasm; mkdir -p $(obj)/gen/third_party/yasm; python source/patched-yasm/modules/arch/x86/gen_x86_insn.py "$(obj)/gen/third_party/yasm"

$(obj)/gen/third_party/yasm/x86insns.c: obj := $(abs_obj)

$(obj)/gen/third_party/yasm/x86insns.c: builddir := $(abs_builddir)

$(obj)/gen/third_party/yasm/x86insns.c: TOOLSET := $(TOOLSET)
$(obj)/gen/third_party/yasm/x86insns.c: third_party/yasm/source/patched-yasm/modules/arch/x86/gen_x86_insn.py FORCE_DO_CMD
	$(call do_cmd,generate_files_generate_x86_insn)
$(obj)/gen/third_party/yasm/x86insn_gas.gperf $(obj)/gen/third_party/yasm/x86insn_nasm.gperf: $(obj)/gen/third_party/yasm/x86insns.c
$(obj)/gen/third_party/yasm/x86insn_gas.gperf $(obj)/gen/third_party/yasm/x86insn_nasm.gperf: ;

all_deps += $(obj)/gen/third_party/yasm/x86insns.c $(obj)/gen/third_party/yasm/x86insn_gas.gperf $(obj)/gen/third_party/yasm/x86insn_nasm.gperf
action_generate_files_generate_x86_insn_outputs := $(obj)/gen/third_party/yasm/x86insns.c $(obj)/gen/third_party/yasm/x86insn_gas.gperf $(obj)/gen/third_party/yasm/x86insn_nasm.gperf

### Rules for action "generate_version":
quiet_cmd_generate_files_generate_version = ACTION Generating yasm version file: $(obj)/gen/third_party/yasm/version.mac. $@
cmd_generate_files_generate_version = export LD_LIBRARY_PATH=$(builddir)/lib.host:$(builddir)/lib.target:$$LD_LIBRARY_PATH; cd third_party/yasm; mkdir -p $(obj)/gen/third_party/yasm; "$(builddir)/genversion" "$(obj)/gen/third_party/yasm/version.mac"

$(obj)/gen/third_party/yasm/version.mac: obj := $(abs_obj)

$(obj)/gen/third_party/yasm/version.mac: builddir := $(abs_builddir)

$(obj)/gen/third_party/yasm/version.mac: TOOLSET := $(TOOLSET)
$(obj)/gen/third_party/yasm/version.mac: $(builddir)/genversion FORCE_DO_CMD
	$(call do_cmd,generate_files_generate_version)

all_deps += $(obj)/gen/third_party/yasm/version.mac
action_generate_files_generate_version_outputs := $(obj)/gen/third_party/yasm/version.mac


### Generated for rule generate_files_generate_gperf:
$(obj)/gen/third_party/yasm/x86cpu.c: obj := $(abs_obj)

$(obj)/gen/third_party/yasm/x86cpu.c: builddir := $(abs_builddir)

$(obj)/gen/third_party/yasm/x86cpu.c: TOOLSET := $(TOOLSET)
$(obj)/gen/third_party/yasm/x86cpu.c: third_party/yasm/source/patched-yasm/modules/arch/x86/x86cpu.gperf $(builddir)/genperf FORCE_DO_CMD
	$(call do_cmd,generate_files_generate_gperf_0)

all_deps += $(obj)/gen/third_party/yasm/x86cpu.c
cmd_generate_files_generate_gperf_0 = export LD_LIBRARY_PATH=$(builddir)/lib.host:$(builddir)/lib.target:$$LD_LIBRARY_PATH; cd third_party/yasm; mkdir -p $(obj)/gen/third_party/yasm; "$(builddir)/genperf" "$(abspath $<)" "$(obj)/gen/third_party/yasm/x86cpu.c"
quiet_cmd_generate_files_generate_gperf_0 = RULE generate_files_generate_gperf_0 $@

$(obj)/gen/third_party/yasm/x86regtmod.c: obj := $(abs_obj)

$(obj)/gen/third_party/yasm/x86regtmod.c: builddir := $(abs_builddir)

$(obj)/gen/third_party/yasm/x86regtmod.c: TOOLSET := $(TOOLSET)
$(obj)/gen/third_party/yasm/x86regtmod.c: third_party/yasm/source/patched-yasm/modules/arch/x86/x86regtmod.gperf $(builddir)/genperf FORCE_DO_CMD
	$(call do_cmd,generate_files_generate_gperf_1)

all_deps += $(obj)/gen/third_party/yasm/x86regtmod.c
cmd_generate_files_generate_gperf_1 = export LD_LIBRARY_PATH=$(builddir)/lib.host:$(builddir)/lib.target:$$LD_LIBRARY_PATH; cd third_party/yasm; mkdir -p $(obj)/gen/third_party/yasm; "$(builddir)/genperf" "$(abspath $<)" "$(obj)/gen/third_party/yasm/x86regtmod.c"
quiet_cmd_generate_files_generate_gperf_1 = RULE generate_files_generate_gperf_1 $@

rule_generate_files_generate_gperf_outputs := $(obj)/gen/third_party/yasm/x86cpu.c \
	$(obj)/gen/third_party/yasm/x86regtmod.c

### Finished generating for rule: generate_files_generate_gperf

### Finished generating for all rules

DEFS_Default := 

# Flags passed to all source files.
CFLAGS_Default := 

# Flags passed to only C files.
CFLAGS_C_Default := 

# Flags passed to only C++ files.
CFLAGS_CC_Default := 

INCS_Default := 

OBJS := 

# Add to the list of files we specially track dependencies for.
all_deps += $(OBJS)

# Make sure our dependencies are built before any of us.
$(OBJS): | $(builddir)/genperf $(builddir)/genversion

# Make sure our actions/rules run before any of us.
$(OBJS): | $(action_generate_files_generate_x86_insn_outputs) $(action_generate_files_generate_version_outputs) $(rule_generate_files_generate_gperf_outputs)


### Rules for final target.
# Build our special outputs first.
$(obj).host/third_party/yasm/generate_files.stamp: | $(action_generate_files_generate_x86_insn_outputs) $(action_generate_files_generate_version_outputs) $(rule_generate_files_generate_gperf_outputs)

# Preserve order dependency of special output on deps.
$(action_generate_files_generate_x86_insn_outputs): | $(builddir)/genperf $(builddir)/genversion

$(obj).host/third_party/yasm/generate_files.stamp: TOOLSET := $(TOOLSET)
$(obj).host/third_party/yasm/generate_files.stamp: $(builddir)/genperf $(builddir)/genversion FORCE_DO_CMD
	$(call do_cmd,touch)

all_deps += $(obj).host/third_party/yasm/generate_files.stamp
# Add target alias
.PHONY: generate_files
generate_files: $(obj).host/third_party/yasm/generate_files.stamp

# Add target alias to "all" target.
.PHONY: all
all: generate_files

