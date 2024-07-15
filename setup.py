from setuptools import setup, find_packages

setup(
    name='datoviz',
    version='0.2.0',
    packages=find_packages(),
    include_package_data=True,
    package_data={
        'datoviz': [
            'ctypes_wrapper.py',
            '../libs/vulkan/linux/libvulkan.so',
            '../build/libdatoviz.so',
        ],
    },
    zip_safe=False,
)
