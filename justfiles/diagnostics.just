[linux]
swiftshader +args:
    @icd="${VK_ICD_FILENAMES:-/usr/local/share/vulkan/icd.d/swiftshader_icd.json}"; \
    if [ ! -f "$icd" ]; then echo "SwiftShader ICD not found; set VK_ICD_FILENAMES or install SwiftShader outside data/." >&2; exit 1; fi; \
    VK_ICD_FILENAMES="$icd" {{args}}
#

[macos]
swiftshader +args:
    @icd="${VK_ICD_FILENAMES:-$HOME/.cache/datoviz/swiftshader/macos/vk_swiftshader_icd.json}"; \
    if [ ! -f "$icd" ]; then echo "SwiftShader ICD not found; set VK_ICD_FILENAMES or install SwiftShader outside data/." >&2; exit 1; fi; \
    VK_ICD_FILENAMES="$icd" {{args}}
#

[windows]
swiftshader +args:
    VK_ICD_FILENAMES=%VK_ICD_FILENAMES% \
    VK_LOADER_DEBUG=all \
    {{args}}
#


# -------------------------------------------------------------------------------------------------
# Code quality
# -------------------------------------------------------------------------------------------------

format:
    find tests/ src/ include/ -iname *.h -o -iname *.c | xargs clang-format -i
#

valgrind args="":
    # NOTE: need to remove -pg compiler option before running valgrind
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --keep-debuginfo=yes \
        --track-origins=yes \
        --verbose \
        --suppressions=.valgrind.exceptions.txt \
        --log-file=.valgrind.out.txt \
        {{args}}
#

cppcheck:
    cppcheck --enable=all --inconclusive src/ include/ cli/ tests/ -i external -I include/datoviz
    # 2> .cppcheck.out.txt && \
    # echo ".cppcheck.out.txt saved"
#

prof:
    gprof build/datoviz gmon.out
#

tree:
    tree -P '*.c' -P '*.h' -P '*.py' -P '*.cpp' -P '*.glsl' -P '*.vert' -P '*.frag' include src datoviz tests pytests examples cli
#

cloc:
    cloc include src datoviz tests pytests examples cli --quiet
#

copyright:
    #!/bin/bash

    # Define the copyright text
    COPYRIGHT_TEXT="/*
     * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
     * Licensed under the MIT license. See LICENSE file in the project root for details.
     * SPDX-License-Identifier: MIT
     */
    "

    # Define the directories to search through
    DIRECTORIES=("tests" "src" "include" "cli")

    # Define the file extensions to look for
    EXTENSIONS=("comp" "vert" "frag" "glsl" "c" "h")

    # Function to prepend text to a file
    prepend_text() {
        local file="$1"
        # Check if the file already contains the copyright text
        if ! grep -q "SPDX-License-Identifier" "$file"; then
            # Prepend the copyright text to the file
            echo $file
            { echo "$COPYRIGHT_TEXT"; cat "$file"; } > temp_file && mv temp_file "$file"
        fi
    }

    # Loop through each directory
    for dir in "${DIRECTORIES[@]}"; do
        # Loop through each extension
        for ext in "${EXTENSIONS[@]}"; do
            # Find all files with the current extension in the current directory
            find "$dir" -type f -name "*.$ext" | while read -r file; do
                prepend_text "$file"
            done
        done
    done
#

# -------------------------------------------------------------------------------------------------
# Static analysis
# -------------------------------------------------------------------------------------------------

analyze:
    @if [ ! -f build/compile_commands.json ]; then \
        echo "compile_commands.json not found. Run 'just build' first."; \
        exit 1; \
    fi
    @if command -v run-clang-tidy >/dev/null 2>&1; then \
        ROOT=$(pwd); \
        FILES=$(jq -r '.[].file' build/compile_commands.json \
            | grep -E "^$ROOT/(include|src|testing)/" \
            | grep -v '\.cu$' \
            | grep -v 'dvz_public_header_probe\.c$' \
            | tr '\n' ' '); \
        if [ -z "$FILES" ]; then \
            echo "No files matched for static analysis."; \
        else \
            run-clang-tidy -p build -quiet -header-filter="^$ROOT/(include|src|testing)/" $FILES; \
        fi; \
    else \
        echo "run-clang-tidy not found. Install clang-tidy or add it to PATH."; \
        exit 1; \
    fi
#

[linux]
exec arg:
    @LD_LIBRARY_PATH=build/ python3 -c "import ctypes; print(ctypes.cdll.LoadLibrary('libdatoviz.so').{{arg}}())"
#



# -------------------------------------------------------------------------------------------------
# Demo
# -------------------------------------------------------------------------------------------------

# demo:
#     ./build/datoviz demo
# #

[linux]
pydemo_dll:
    @LD_LIBRARY_PATH=build/ python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.so').dvz_demo()"
#

[macos]
pydemo_dll:
    @DYLD_LIBRARY_PATH=build/ python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dylib').dvz_demo()"
#

[windows]
pydemo_dll:
    python -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dll').dvz_demo()"
#

pydemo:
    python -c "import datoviz; datoviz.demo()"
#

python *args:
