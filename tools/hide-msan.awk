# HACK: remove Vulkan loader spurious memory sanitizer warnings and only keep genuine Datoviz ones
# HACK: need to bring back the colors manually as they are discarded by awk

BEGIN {
    RED   = "\033[1;31m"
    PURPLE= "\033[1;35m"
    RESET = "\033[0m"
}

# Hide trivial "Uninitialized bytes" noise lines
/^Uninitialized bytes / { next }

# Detect start of a MSan warning block
/^==[0-9]+==WARNING: MemorySanitizer/ {
    warn=1
    block = RED $0 RESET ORS
    in_loader=0
    next
}

# Collect lines while inside a warning block
warn {
    # Highlight 'Uninitialized value...' lines in purple
    if ($0 ~ /Uninitialized value/) {
        block = block PURPLE $0 RESET ORS
    } else {
        block = block $0 ORS
    }

    if ($0 ~ /Vulkan-Loader/) in_loader=1

    # End of block
    if ($0 ~ /^SUMMARY:/) {
        if (!in_loader) printf "%s", block
        warn=0; block=""; in_loader=0
    }
    next
}

# Pass everything else through untouched
!warn
