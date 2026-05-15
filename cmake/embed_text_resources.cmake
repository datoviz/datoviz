# Embed text files as null-terminated C byte arrays.
#
# Invoked by CMake add_custom_command with -P flag:
#   cmake -D FILES="file1;file2;..." -D PREFIX=wgsl -D OUTPUT=path/_wgsl.c
#         -P embed_text_resources.cmake
#
# Generates a .c file with:
#   static const unsigned char DVZ_RESOURCE_<PREFIX>_<name>[] = { ..., 0x00 };
#   static const unsigned long DVZ_RESOURCE_<PREFIX>_<name>_size = N;
#
# and a lookup function:
#   const char* dvz_resource_<prefix>(const char* name, unsigned long* size);
#
# <name> is derived from the filename with extension stripped and non-alnum chars -> '_'.

function(create_text_resources files prefix output)
    file(WRITE "${output}" "")
    file(APPEND "${output}" "#include <string.h>\n")
    file(APPEND "${output}" "#include \"datoviz/fileio/fileio.h\"\n\n")

    foreach(txt IN LISTS files)
        if(NOT EXISTS "${txt}")
            message(WARNING "embed_text_resources: ${txt} does not exist, skipping")
            continue()
        endif()
        get_filename_component(filename "${txt}" NAME)
        if(filename MATCHES "\\.(vert|frag|comp)$")
            set(stem "${filename}")
        else()
            string(REGEX REPLACE "\\.[^.]*$" "" stem "${filename}")
        endif()
        string(REGEX REPLACE "[^A-Za-z0-9]" "_" cname "${stem}")

        file(READ "${txt}" filedata HEX)
        file(SIZE "${txt}" filesize)
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata "${filedata}")

        file(APPEND "${output}" "static const unsigned char DVZ_RESOURCE_${prefix}_${cname}[] = {${filedata}0x00};\n")
        file(APPEND "${output}" "static const unsigned long DVZ_RESOURCE_${prefix}_${cname}_size = ${filesize}UL;\n\n")
    endforeach()

    file(APPEND "${output}" "const char* dvz_resource_${prefix}(const char* name, unsigned long* size)\n{\n")
    file(APPEND "${output}" "    if (size) *size = 0;\n")
    foreach(txt IN LISTS files)
        if(NOT EXISTS "${txt}")
            continue()
        endif()
        get_filename_component(filename "${txt}" NAME)
        if(filename MATCHES "\\.(vert|frag|comp)$")
            set(stem "${filename}")
        else()
            string(REGEX REPLACE "\\.[^.]*$" "" stem "${filename}")
        endif()
        string(REGEX REPLACE "[^A-Za-z0-9]" "_" cname "${stem}")

        file(APPEND "${output}" "    if (strcmp(name, \"${cname}\") == 0) {\n")
        file(APPEND "${output}" "        if (size) *size = DVZ_RESOURCE_${prefix}_${cname}_size;\n")
        file(APPEND "${output}" "        return (const char*)DVZ_RESOURCE_${prefix}_${cname};\n")
        file(APPEND "${output}" "    }\n")
    endforeach()
    file(APPEND "${output}" "    return 0;\n}\n")
endfunction()

create_text_resources("${FILES}" "${PREFIX}" "${OUTPUT}")
