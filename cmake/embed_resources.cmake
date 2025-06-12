# Creates C resources file from files in given directory
# from: https://stackoverflow.com/a/27206982
function(create_resources files prefix output)
    # Create empty output file
    file(WRITE ${output} "")
    file(APPEND ${output} "#include <string.h>\n")
    file(APPEND ${output} "#include \"../include/datoviz_macros.h\"\n")
    file(APPEND ${output} "#include \"../include/datoviz/common.h\"\n")
    file(APPEND ${output} "#include \"../include/datoviz/fileio.h\"\n")

    # Collect input files
    # HACK: if "files" has a * in it, it should be interpreted as a GLOB
    # otherwise, it is a list of files
    # We need this distinction because for shaders, the glob must be done at call time rathen than
    # at CMakeLists.txt evaluation time since the shaders are first compiled by glslc by CMake.
    if(<${files}|string> MATCHES "\\*")
        file(GLOB files_l ${files})
    else()
        # Iterate through input files
        set(files_l ${files})
        separate_arguments(files_l)
    endif()

    foreach(bin ${files_l})
        # Get short filename
        string(REGEX MATCH "([^/]+)$" filename ${bin})

        # HACK: do not include non graphics shaders in the embeded resources files.
        if(${filename} MATCHES ".spv" AND ${filename} MATCHES "test_")
            continue()
        endif()

        # Remove the file extension
        string(REGEX REPLACE "\\.[^.]*$" "" filename ${filename})

        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "\\.| |-" "_" filename ${filename})

        # Read hex data from file
        file(READ ${bin} filedata HEX)
        file(SIZE ${bin} filesize)

        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})

        # Append data to output file
        file(APPEND ${output} "unsigned char DVZ_RESOURCE_${prefix}_${filename}[] = {${filedata}};\n")
        file(APPEND ${output} "unsigned long DVZ_RESOURCE_${prefix}_${filename}_size = ${filesize};\n")
    endforeach()

    # Loading function.
    file(APPEND ${output} "unsigned char* dvz_resource_${prefix}(const char* name, unsigned long* size)\n")
    file(APPEND ${output} "{\n")

    # file(APPEND ${output} "printf(\"%s\\n\", name);\n")
    # Iterate through input files
    foreach(bin ${files_l})
        string(REGEX MATCH "([^/]+)$" filename ${bin})

        # HACK: do not include non graphics shaders in the embeded resources files.
        if(${filename} MATCHES ".spv" AND ${filename} MATCHES "test_")
            continue()
        endif()

        string(REGEX REPLACE "\\.[^.]*$" "" filename ${filename})
        string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
        file(APPEND ${output} "if (strcmp(name, \"${filename}\") == 0) {*size = DVZ_RESOURCE_${prefix}_${filename}_size; return DVZ_RESOURCE_${prefix}_${filename};}\n")
    endforeach()

    file(APPEND ${output} "if (*size == 0) log_error(\"unable to find file %s\", name);\n")
    file(APPEND ${output} "return NULL;}\n")
endfunction()

create_resources(${DIR} ${PREFIX} ${OUTPUT})
