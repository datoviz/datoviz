# Embed text files as null-terminated C byte arrays.
#
# Invoked by CMake add_custom_command with -P flag:
#   cmake -D FILES="file1;file2;..." -D PREFIX=wgsl -D OUTPUT=path/_wgsl.c
#         -P embed_text_resources.cmake
#
# Generates a .c file with:
#   static const unsigned char DVZ_RESOURCE_<PREFIX>_<name>[] = { ..., 0x00 };
#   static const DvzSize DVZ_RESOURCE_<PREFIX>_<name>_size = N;
#
# and a lookup function:
#   const char* dvz_resource_<prefix>(const char* name, DvzSize* size);
#
# <name> is derived from the filename with extension stripped and non-alnum chars -> '_'.

function(read_text_with_local_includes input output_data output_size)
    file(READ "${input}" data)
    get_filename_component(input_dir "${input}" DIRECTORY)

    string(REGEX MATCHALL "#include[ \t]+\"[^\"]+\"" includes "${data}")
    foreach(include_stmt IN LISTS includes)
        string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" include_name "${include_stmt}")
        set(include_path "${input_dir}/${include_name}")
        if(EXISTS "${include_path}")
            file(READ "${include_path}" include_data)
            string(REPLACE "${include_stmt}" "${include_data}" data "${data}")
        else()
            message(WARNING "embed_text_resources: include ${include_path} does not exist")
        endif()
    endforeach()

    string(LENGTH "${data}" data_size)
    set(${output_data} "${data}" PARENT_SCOPE)
    set(${output_size} "${data_size}" PARENT_SCOPE)
endfunction()

function(create_text_resources files prefix output)
    # Custom-command argument escaping preserves list separators as "\\;" on Windows.
    # Normalize them before using IN LISTS so every generated shader becomes a resource.
    string(REPLACE "\\;" ";" files "${files}")
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

        read_text_with_local_includes("${txt}" filetext filesize)
        string(HEX "${filetext}" filedata)
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata "${filedata}")

        file(APPEND "${output}" "static const unsigned char DVZ_RESOURCE_${prefix}_${cname}[] = {${filedata}0x00};\n")
        file(APPEND "${output}" "static const DvzSize DVZ_RESOURCE_${prefix}_${cname}_size = ${filesize}ULL;\n\n")
    endforeach()

    file(APPEND "${output}" "DVZ_EXPORT const char* dvz_resource_${prefix}(const char* name, DvzSize* size)\n{\n")
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
