import platform
from setuptools import setup, find_packages

packages = [
    '__init__.py',
]

packages.extend(
    {
        "Darwin": [
            "../build/libdatoviz*.dylib",
            "../libs/vulkan/macos/libvulkan*.dylib",
        ],
        "Linux": [
            "../build/libdatoviz*.so",
            "../libs/vulkan/linux/libvulkan*.so",
        ],
        "Windows": [],
    }.get(platform.system(), [])
)

setup(
    name='datoviz',
    version='0.2.0',
    packages=find_packages(),
    include_package_data=True,
    package_data={
        'datoviz': packages,
    },
    zip_safe=False,
)
