# Resolve preloaded resource files once for linking and build provenance.
function(dvz_wasm_preload source destination)
    if(IS_DIRECTORY "${source}")
        file(GLOB_RECURSE inputs CONFIGURE_DEPENDS LIST_DIRECTORIES false "${source}/*")
    else()
        set(inputs "${source}")
    endif()
    target_link_options(datoviz_wasm_scene PRIVATE
        "SHELL:--preload-file ${source}@${destination}")
    set_property(TARGET datoviz_wasm_scene APPEND PROPERTY LINK_DEPENDS ${inputs})
    set(DVZ_WASM_RESOLVED_INPUTS ${DVZ_WASM_RESOLVED_INPUTS} ${inputs} PARENT_SCOPE)
endfunction()
