#!/bin/sh
self=$0

usage() {
  cat <<EOF >&2
Usage: $self [options] FILE

Reads the Run Time CPU Detections definitions from FILE and generates a
C header file on stdout.

Options:
  --arch=ARCH   Architecture to generate defs for (required)
  --disable-EXT Disable support for EXT extensions
  --require-EXT Require support for EXT extensions
  --sym=SYMBOL  Unique symbol to use for RTCD initialization function
  --config=FILE File with CONFIG_FOO=yes lines to parse
EOF
  exit 1
}

die() {
  echo "$@" >&2
  exit 1
}

die_argument_required() {
  die "Option $opt requires argument"
}

for opt; do
  optval="${opt#*=}"
  case "$opt" in
    --arch) die_argument_required;;
    --arch=*) arch=${optval};;
    --disable-*) eval "disable_${opt#--disable-}=true";;
    --require-*) REQUIRES="${REQUIRES}${opt#--require-} ";;
    --sym) die_argument_required;;
    --sym=*) symbol=${optval};;
    --config=*) config_file=${optval};;
    -h|--help)
      usage
      ;;
    -*)
      die "Unrecognized option: ${opt%%=*}"
      ;;
    *)
      defs_file="$defs_file $opt"
      ;;
  esac
  shift
done
for f in $defs_file; do [ -f "$f" ] || usage; done
[ -n "$arch" ] || usage

# Import the configuration
[ -f "$config_file" ] && eval $(grep CONFIG_ "$config_file")

#
# Routines for the RTCD DSL to call
#
prototype() {
  rtyp=""
  case "$1" in
    unsigned) rtyp="$1 "; shift;;
  esac
  rtyp="${rtyp}$1"
  fn="$2"
  args="$3"

  eval "${2}_rtyp='$rtyp'"
  eval "${2}_args='$3'"
  ALL_FUNCS="$ALL_FUNCS $fn"
  specialize $fn c
}

specialize() {
  fn="$1"
  shift
  for opt in "$@"; do
    eval "${fn}_${opt}=${fn}_${opt}"
  done
}

require() {
  for fn in $ALL_FUNCS; do
    for opt in "$@"; do
      ofn=$(eval "echo \$${fn}_${opt}")
      [ -z "$ofn" ] && continue

      # if we already have a default, then we can disable it, as we know
      # we can do better.
      best=$(eval "echo \$${fn}_default")
      best_ofn=$(eval "echo \$${best}")
      [ -n "$best" ] && [ "$best_ofn" != "$ofn" ] && eval "${best}_link=false"
      eval "${fn}_default=${fn}_${opt}"
      eval "${fn}_${opt}_link=true"
    done
  done
}

forward_decls() {
  ALL_FORWARD_DECLS="$ALL_FORWARD_DECLS $1"
}

#
# Include the user's directives
#
for f in $defs_file; do
  . $f
done

#
# Process the directives according to the command line
#
process_forward_decls() {
  for fn in $ALL_FORWARD_DECLS; do
    eval $fn
  done
}

determine_indirection() {
  [ "$CONFIG_RUNTIME_CPU_DETECT" = "yes" ] || require $ALL_ARCHS
  for fn in $ALL_FUNCS; do
    n=""
    rtyp="$(eval "echo \$${fn}_rtyp")"
    args="$(eval "echo \"\$${fn}_args\"")"
    dfn="$(eval "echo \$${fn}_default")"
    dfn=$(eval "echo \$${dfn}")
    for opt in "$@"; do
      ofn=$(eval "echo \$${fn}_${opt}")
      [ -z "$ofn" ] && continue
      link=$(eval "echo \$${fn}_${opt}_link")
      [ "$link" = "false" ] && continue
      n="${n}x"
    done
    if [ "$n" = "x" ]; then
      eval "${fn}_indirect=false"
    else
      eval "${fn}_indirect=true"
    fi
  done
}

declare_function_pointers() {
  for fn in $ALL_FUNCS; do
    rtyp="$(eval "echo \$${fn}_rtyp")"
    args="$(eval "echo \"\$${fn}_args\"")"
    dfn="$(eval "echo \$${fn}_default")"
    dfn=$(eval "echo \$${dfn}")
    for opt in "$@"; do
      ofn=$(eval "echo \$${fn}_${opt}")
      [ -z "$ofn" ] && continue
      echo "$rtyp ${ofn}($args);"
    done
    if [ "$(eval "echo \$${fn}_indirect")" = "false" ]; then
      echo "#define ${fn} ${dfn}"
    else
      echo "RTCD_EXTERN $rtyp (*${fn})($args);"
    fi
    echo
  done
}

set_function_pointers() {
  for fn in $ALL_FUNCS; do
    n=""
    rtyp="$(eval "echo \$${fn}_rtyp")"
    args="$(eval "echo \"\$${fn}_args\"")"
    dfn="$(eval "echo \$${fn}_default")"
    dfn=$(eval "echo \$${dfn}")
    if $(eval "echo \$${fn}_indirect"); then
      echo "    $fn = $dfn;"
      for opt in "$@"; do
        ofn=$(eval "echo \$${fn}_${opt}")
        [ -z "$ofn" ] && continue
        [ "$ofn" = "$dfn" ] && continue;
        link=$(eval "echo \$${fn}_${opt}_link")
        [ "$link" = "false" ] && continue
        cond="$(eval "echo \$have_${opt}")"
        echo "    if (${cond}) $fn = $ofn;"
      done
    fi
    echo
  done
}

filter() {
  filtered=""
  for opt in "$@"; do
    [ -z $(eval "echo \$disable_${opt}") ] && filtered="$filtered $opt"
  done
  echo $filtered
}

#
# Helper functions for generating the arch specific RTCD files
#
common_top() {
  outfile_basename=$(basename ${symbol:-rtcd})
  include_guard=$(echo $outfile_basename | tr '[a-z]' '[A-Z]' | \
    tr -c '[A-Z0-9]' _)H_
  cat <<EOF
#ifndef ${include_guard}
#define ${include_guard}

#ifdef RTCD_C
#define RTCD_EXTERN
#else
#define RTCD_EXTERN extern
#endif

$(process_forward_decls)

$(declare_function_pointers c $ALL_ARCHS)

void ${symbol:-rtcd}(void);
EOF
}

common_bottom() {
  cat <<EOF
#endif
EOF
}

x86() {
  determine_indirection c $ALL_ARCHS

  # Assign the helper variable for each enabled extension
  for opt in $ALL_ARCHS; do
    uc=$(echo $opt | tr '[a-z]' '[A-Z]')
    eval "have_${opt}=\"flags & HAS_${uc}\""
  done

  cat <<EOF
$(common_top)

#ifdef RTCD_C
#include "vpx_ports/x86.h"
static void setup_rtcd_internal(void)
{
    int flags = x86_simd_caps();

    (void)flags;

$(set_function_pointers c $ALL_ARCHS)
}
#endif
$(common_bottom)
EOF
}

arm() {
  determine_indirection c $ALL_ARCHS

  # Assign the helper variable for each enabled extension
  for opt in $ALL_ARCHS; do
    uc=$(echo $opt | tr '[a-z]' '[A-Z]')
    eval "have_${opt}=\"flags & HAS_${uc}\""
  done

  cat <<EOF
$(common_top)
#include "vpx_config.h"

#ifdef RTCD_C
#include "vpx_ports/arm.h"
static void setup_rtcd_internal(void)
{
    int flags = arm_cpu_caps();

    (void)flags;

$(set_function_pointers c $ALL_ARCHS)
}
#endif
$(common_bottom)
EOF
}


mips() {
  determine_indirection c $ALL_ARCHS
  cat <<EOF
$(common_top)
#include "vpx_config.h"

#ifdef RTCD_C
static void setup_rtcd_internal(void)
{
$(set_function_pointers c $ALL_ARCHS)
#if HAVE_DSPR2
void dsputil_static_init();
dsputil_static_init();
#endif
}
#endif
$(common_bottom)
EOF
}

unoptimized() {
  determine_indirection c
  cat <<EOF
$(common_top)
#include "vpx_config.h"

#ifdef RTCD_C
static void setup_rtcd_internal(void)
{
$(set_function_pointers c)
}
#endif
$(common_bottom)
EOF

}
#
# Main Driver
#
require c
case $arch in
  x86)
    ALL_ARCHS=$(filter mmx sse sse2 sse3 ssse3 sse4_1)
    x86
    ;;
  x86_64)
    ALL_ARCHS=$(filter mmx sse sse2 sse3 ssse3 sse4_1)
    REQUIRES=${REQUIRES:-mmx sse sse2}
    require $(filter $REQUIRES)
    x86
    ;;
  mips32)
    ALL_ARCHS=$(filter mips32)
    dspr2=$([ -f "$config_file" ] && eval echo $(grep HAVE_DSPR2 "$config_file"))
    HAVE_DSPR2="${dspr2#*=}"
    if [ "$HAVE_DSPR2" = "yes" ]; then
        ALL_ARCHS=$(filter mips32 dspr2)
    fi
    mips
    ;;
  armv5te)
    ALL_ARCHS=$(filter edsp)
    arm
    ;;
  armv6)
    ALL_ARCHS=$(filter edsp media)
    arm
    ;;
  armv7)
    ALL_ARCHS=$(filter edsp media neon)
    arm
    ;;
  *)
    unoptimized
    ;;
esac
