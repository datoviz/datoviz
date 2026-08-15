default:
    @echo "No arguments supplied"
    @exit 1
#

check-staged:
    python3 tools/check_staged_payloads.py
#

font-assets-check:
    python3 tools/check_runtime_fonts.py
#

install-git-hooks:
    #!/usr/bin/env sh
    set -e
    mkdir -p .git/hooks
    {
        printf '%s\n' '#!/usr/bin/env sh'
        printf '%s\n' 'set -e'
        printf '%s\n' 'python3 tools/check_staged_payloads.py'
    } > .git/hooks/pre-commit
    chmod +x .git/hooks/pre-commit
    echo "Installed .git/hooks/pre-commit"
#



# -------------------------------------------------------------------------------------------------
# Templates
# -------------------------------------------------------------------------------------------------

stub path:
    #!/usr/bin/env sh
    path="{{path}}"

    # Determine extension and base name
    ext="${path##*.}"
    base="$(basename "$path")"

    # Pick template based on name pattern or extension
    template=""
    case "$base" in
        *_structs.h) template="{{TEMPLATES_DIR}}/structs.h" ;;
        test*.c)      template="{{TEMPLATES_DIR}}/test.c" ;;
        test*.h)      template="{{TEMPLATES_DIR}}/test.h" ;;
        *.h)          template="{{TEMPLATES_DIR}}/header.h" ;;
        *.c)          template="{{TEMPLATES_DIR}}/source.c" ;;
        *) echo "error: no template defined for $path" >&2; exit 1 ;;
    esac

    mkdir -p "$(dirname "$path")"

    # Extract filename without extension
    title="$(basename "$path" ".$ext")"

    # Split by '_' and take the last part
    title="${title##*_}"

    # Lowercase (POSIX way via tr)
    title="$(printf '%s' "$title" | tr '[:upper:]' '[:lower:]')"

    # Replace <title> placeholder
    sed "s/<title>/$title/g" "$template" > "$path"

    echo "Created stub: $path"
#



clean:
    @rm -rf build*
#

rebuild:
    just clean
    just build || just build
#

rmbuild:
    @rm -rf build/spirv build/artifacts build/struct_sizes* build/*.dylib* build/*.so* build/*.dll build/datoviz*
#
