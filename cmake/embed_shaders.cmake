# Creates C resources file from files in given directory
# from: https://stackoverflow.com/a/27206982
function(create_resources dir prefix output)
    # Create empty output file
    file(WRITE ${output} "")
    # Collect input files
    file(GLOB bins ${dir}/*)
    # Iterate through input files
    foreach(bin ${bins})
        # Get short filename
        string(REGEX MATCH "([^/]+)$" filename ${bin})
        # Remove the file extension
        string(REGEX REPLACE "\\.[^.]*$" "" filename ${filename})
        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
        # To uppercase
        # string(TOUPPER ${filename} filename)
        # Read hex data from file
        file(READ ${bin} filedata HEX)
        file(SIZE ${bin} filesize)
        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
        # Append data to output file
        file(APPEND ${output} "const unsigned char VKL_BINARY_${prefix}${filename}[] = {${filedata}};\n")
        file(APPEND ${output} "const unsigned long VKL_BINARY_${prefix}${filename}_size = ${filesize};\n")
    endforeach()
endfunction()

create_resources(${SPIRV_DIR} "SHADER_" ${SHADERC_PATH})
