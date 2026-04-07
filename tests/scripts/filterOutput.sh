#!/bin/bash -e

# filter the gcda mismatch messages and the extra output of the asan builds
cat  $1 \
    | grep -v "gcda:Merge mismatch" \
    | grep -v "^==[0-9]*=="| grep -v "^||.*||.*||$" \
    | grep -v "^MemToShadow(shadow):" | grep -v "^redzone=" \
    | grep -v "^max_redzone=" | grep -v "^quarantine_size_mb=" \
    | grep -v "^thread_local_quarantine_size_kb=" \
    | grep -v "^malloc_context_size=" | grep -v "^SHADOW_SCALE:" \
    | grep -v "^SHADOW_GRANULARITY:" | grep -v "SHADOW_OFFSET:" \
    | grep -v "AddressSanitizer: reading suppressions file"
