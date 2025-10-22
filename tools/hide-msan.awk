# Hide trivial "Uninitialized bytes" noise lines
/^Uninitialized bytes / { next }

# Detect start of a MSan warning block
/^==[0-9]+==WARNING: MemorySanitizer/ {
    warn=1
    block=$0 ORS
    in_loader=0
    next
}

# Collect lines while inside a warning block
warn {
    block = block $0 ORS
    if ($0 ~ /Vulkan-Loader/) in_loader=1
    if ($0 ~ /^SUMMARY:/) {
        if (!in_loader) printf "%s", block
        warn=0; block=""; in_loader=0
    }
    next
}

# Pass everything else through
!warn
