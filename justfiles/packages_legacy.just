# -------------------------------------------------------------------------------------------------
# Legacy packaging commands quarantined for v0.4.
# -------------------------------------------------------------------------------------------------

[linux]
deb:
    @echo "Legacy .deb recipe is quarantined for v0.4. Use just distribution-preflight-local and the source-bundle/conda/vcpkg release path." >&2
    @exit 2
#

[linux]
testdeb:
    @echo "Legacy .deb test recipe is quarantined for v0.4. Use just distribution-preflight-local." >&2
    @exit 2
#

[linux]
testwheel:
    @echo "Legacy Docker wheel test is quarantined. Use just wheel-check or just wheel-ci-local <platform-tag>." >&2
    @exit 2
#

[macos]
bundledeps lib="build/libdatoviz.dylib":
    @echo "Legacy macOS dependency bundling is quarantined for v0.4 release work. Use wheel repair/source-bundle validation instead." >&2
    @exit 2
#

[macos]
pkg:
    @echo "Legacy macOS .pkg recipe is quarantined for v0.4. Use wheels/source-bundle distribution checks." >&2
    @exit 2
#

[macos]
testpkg vm_ip_address:
    @echo "Legacy macOS .pkg VM test is quarantined for v0.4." >&2
    @exit 2
#

[macos]
testwheel vm_ip_address="":
    @echo "Legacy VM wheel test is quarantined. Use just wheel-check or just wheel-ci-local <platform-tag>." >&2
    @exit 2
#
