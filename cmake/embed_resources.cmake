# Embed binary files as C byte arrays.
#
# Invoked by CMake add_custom_command with -P flag:
#   cmake -D FILES="file1;file2;..." -D PREFIX=shader -D OUTPUT=path/_shaders.c
#         -P embed_resources.cmake
#
# Generates a .c file with:
#   static const unsigned char DVZ_RESOURCE_<PREFIX>_<name>[] = { ... };
#   static const unsigned long DVZ_RESOURCE_<PREFIX>_<name>_size = N;
#
# and a lookup function:
#   const unsigned char* dvz_resource_<prefix>(const char* name, unsigned long* size);
#
# <name> is derived from the filename with extension stripped and non-alnum chars → '_'.

function(create_resources files prefix output)
    file(WRITE "${output}" "")
    file(APPEND "${output}" "#include <string.h>\n")
    file(APPEND "${output}" "#include \"datoviz/fileio/fileio.h\"\n\n")

    # FILES is passed as a semicolon-separated CMake list via -D FILES="..."
    # Use IN LISTS to iterate properly.
    foreach(bin IN LISTS files)
        if(NOT EXISTS "${bin}")
            message(WARNING "embed_resources: ${bin} does not exist, skipping")
            continue()
        endif()
        get_filename_component(filename "${bin}" NAME)
        string(REGEX REPLACE "\\.[^.]*$" "" stem "${filename}")
        string(REGEX REPLACE "[^A-Za-z0-9]" "_" cname "${stem}")

        file(READ "${bin}" filedata HEX)
        file(SIZE "${bin}" filesize)
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata "${filedata}")

        file(APPEND "${output}" "static const unsigned char DVZ_RESOURCE_${prefix}_${cname}[] = {${filedata}};\n")
        file(APPEND "${output}" "static const unsigned long DVZ_RESOURCE_${prefix}_${cname}_size = ${filesize}UL;\n\n")
    endforeach()

    file(APPEND "${output}" "const unsigned char* dvz_resource_${prefix}(const char* name, unsigned long* size)\n{\n")
    file(APPEND "${output}" "    if (size) *size = 0;\n")
    foreach(bin IN LISTS files)
        if(NOT EXISTS "${bin}")
            continue()
        endif()
        get_filename_component(filename "${bin}" NAME)
        string(REGEX REPLACE "\\.[^.]*$" "" stem "${filename}")
        string(REGEX REPLACE "[^A-Za-z0-9]" "_" cname "${stem}")

        file(APPEND "${output}" "    if (strcmp(name, \"${cname}\") == 0) {\n")
        file(APPEND "${output}" "        if (size) *size = DVZ_RESOURCE_${prefix}_${cname}_size;\n")
        file(APPEND "${output}" "        return DVZ_RESOURCE_${prefix}_${cname};\n")
        file(APPEND "${output}" "    }\n")
    endforeach()
    file(APPEND "${output}" "    return 0;\n}\n")
endfunction()

create_resources("${FILES}" "${PREFIX}" "${OUTPUT}")
