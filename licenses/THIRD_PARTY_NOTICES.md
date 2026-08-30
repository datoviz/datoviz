# Datoviz third-party notices

The packaging manifest [`THIRD_PARTY_LICENSES.txt`](THIRD_PARTY_LICENSES.txt) is the authoritative inventory of license texts copied into every Datoviz C/C++ install and Python wheel. Package license directories preserve each listed source-root-relative path so that attribution and the exact upstream text remain reviewable.

The inventory covers vendored libraries and embedded font assets with standalone license authorities. Public Vulkan, Volk, and VMA headers retain their upstream copyright and license notices in the installed headers themselves. System package-manager dependencies and platform runtime libraries remain owned and licensed by their provider packages; they are not represented as Datoviz-vendored payloads.

Validate the inventory before packaging with:

```sh
python3 tools/check_third_party_notices.py
```
