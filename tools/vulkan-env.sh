#!/usr/bin/env bash

# Source this script to configure the Vulkan SDK environment locally.
# It detects the most recent SDK under ~/VulkanSDK unless VULKAN_SDK is already set.

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "datoviz: this script must be sourced (e.g. 'source tools/vulkan-env.sh')" >&2
    exit 1
fi

_dvz_uname="$(uname)"

# Honour an existing VULKAN_SDK if it already points to a valid SDK directory.
if [ -n "${VULKAN_SDK:-}" ] && [ -d "${VULKAN_SDK}" ]; then
    _dvz_vulkan_sdk_root="${VULKAN_SDK}"
fi

if [ "${_dvz_uname}" != "Darwin" ] && [ -n "${_dvz_vulkan_sdk_root:-}" ]; then
    case "${_dvz_vulkan_sdk_root}" in
        */VulkanSDK/*) unset _dvz_vulkan_sdk_root ;;
    esac
fi

if [ -z "${_dvz_vulkan_sdk_root:-}" ]; then
    if [ "${_dvz_uname}" = "Darwin" ]; then
        _dvz_candidate=$(ls -1d "${HOME}/VulkanSDK"/*/macOS 2>/dev/null | sort | tail -n 1)
    else
        # Prefer the LunarG Linux layout introduced with 1.3.290 (~/Vulkan/<ver>/x86_64).
        _dvz_candidate=$(ls -1d "${HOME}/Vulkan"/*/x86_64 2>/dev/null | sort | tail -n 1)

        if [ -z "${_dvz_candidate}" ]; then
            _dvz_candidate=$(ls -1d "${HOME}/VulkanSDK"/*/x86_64 2>/dev/null | sort | tail -n 1)
        fi
    fi

    if [ -z "${_dvz_candidate}" ]; then
        echo "datoviz: no Vulkan SDK installation found under \"${HOME}/Vulkan\" or \"${HOME}/VulkanSDK\"" >&2
        echo "datoviz: install the SDK from https://vulkan.lunarg.com/sdk/home and retry" >&2
        return 1
    fi

    _dvz_vulkan_sdk_root="${_dvz_candidate}"
fi

export VULKAN_SDK="${_dvz_vulkan_sdk_root}"

_dvz_layer_dir="${VULKAN_SDK}/share/vulkan/explicit_layer.d"

if [ ! -d "${_dvz_layer_dir}" ]; then
    _dvz_layer_dir="${VULKAN_SDK}/etc/vulkan/explicit_layer.d"
fi

if [ -d "${_dvz_layer_dir}" ]; then
    export VK_LAYER_PATH="${_dvz_layer_dir}"
fi

# Ensure the SDK binaries and libraries come first; avoid duplicate separators.
case ":${PATH}:" in
    *:"${VULKAN_SDK}/bin":*) ;;
    *) export PATH="${VULKAN_SDK}/bin:${PATH}" ;;
esac

if [ "${_dvz_uname}" = "Darwin" ]; then
    _dvz_icd_path="${VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json"

    if [ ! -f "${_dvz_icd_path}" ]; then
        _dvz_icd_path="${VULKAN_SDK}/etc/vulkan/icd.d/MoltenVK_icd.json"
    fi

    export VK_ICD_FILENAMES="${_dvz_icd_path}"

    if [ -n "${DYLD_LIBRARY_PATH:-}" ]; then
        case ":${DYLD_LIBRARY_PATH}:" in
            *:"${VULKAN_SDK}/lib":*) ;;
            *) export DYLD_LIBRARY_PATH="${VULKAN_SDK}/lib:${DYLD_LIBRARY_PATH}" ;;
        esac
    else
        export DYLD_LIBRARY_PATH="${VULKAN_SDK}/lib"
    fi
else
    if [ -n "${LD_LIBRARY_PATH:-}" ]; then
        case ":${LD_LIBRARY_PATH}:" in
            *:"${VULKAN_SDK}/lib":*) ;;
            *) export LD_LIBRARY_PATH="${VULKAN_SDK}/lib:${LD_LIBRARY_PATH}" ;;
        esac
    else
        export LD_LIBRARY_PATH="${VULKAN_SDK}/lib"
    fi
fi

echo "datoviz: VULKAN_SDK set to ${VULKAN_SDK}" >&2
if [ -n "${VK_LAYER_PATH:-}" ]; then
    echo "datoviz: VK_LAYER_PATH set to ${VK_LAYER_PATH}" >&2
fi
if [ -n "${VK_ICD_FILENAMES:-}" ]; then
    echo "datoviz: VK_ICD_FILENAMES set to ${VK_ICD_FILENAMES}" >&2
fi

unset _dvz_vulkan_sdk_root
unset _dvz_candidate
unset _dvz_layer_dir
unset _dvz_icd_path
unset _dvz_uname
