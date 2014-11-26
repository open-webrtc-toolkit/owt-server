#!/bin/bash
##
##  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
##
##  Use of this source code is governed by a BSD-style license
##  that can be found in the LICENSE file in the root of the source
##  tree. An additional intellectual property rights grant can be found
##  in the file PATENTS.  All contributing project authors may
##  be found in the AUTHORS file in the root of the source tree.
##


# gen_example_code.sh

self=$0

die_usage() {
    echo "Usage: $self <example.txt>"
    exit 1
}

die() {
    echo "$@"
    exit 1
}

include_block() {
    show_bar=$1
    block_name=${line##*@}
    indent=${line%%${block_name}}
    indent=${#indent}
    [ $indent -eq 1 ] && indent=0
    local on_block
    while IFS=$'\n' read -r t_line; do
        case "$t_line" in
            \~*\ ${block_name})
                if [ "x$on_block" == "xyes" ]; then
                    return 0;
                else
                    on_block=yes
                fi
                ;;
            @DEFAULT)
                if [ "x$on_block" == "xyes" ]; then
                    include_block $show_bar < "${template%.c}.txt"
                    return 0
                fi
                ;;
            *)
                if [ "x$on_block" == "xyes" ]; then
                    local rem
                    (( rem = 78 - indent ))
                    case "$block_name" in
                      \**) printf "%${indent}s * %s\n" "" "$t_line" ;;
                        *)
                            if [ "$show_bar" == "yes" ]; then
                                printf "%${indent}s%-${rem}s//\n" "" "$t_line"
                            else
                                printf "%${indent}s%s\n" "" "$t_line"
                            fi
                            ;;
                    esac
                fi
        esac
    done
    return 1
}

txt=$1
[ -f "$txt" ] || die_usage
read -r template < "$txt"
case "$template" in
    @TEMPLATE*) template=${txt%/*}/${template##@TEMPLATE } ;;
    *) die "Failed to parse template name from '$template'" ;;
esac

while IFS=$'\n' read -r line; do
    case "$line" in
        @*) include_block yes < "$txt" \
            || include_block < "${template%.c}.txt" \
            #|| echo "WARNING: failed to find text for block $block_name" >&2
            ;;
        *)  echo "$line" ;;
    esac
done < "$template"
