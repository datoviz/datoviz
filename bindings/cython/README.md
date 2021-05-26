# Datoviz Python bindings

This subdirectory contains early Python bindings of datoviz, using Cython.

To build the module on Linux, go to the datoviz root folder, and do `./manage.sh cython`.

This script essentially does `python3 setup.py build_ext -i` after automatically updating the Cython low-level wrapper with a Python script.

To make a pip-installable wheel on Linux, go to the datoviz root folder, and do `./manage.sh wheel`. This script uses auditwheel to bundle libdatoviz as a dependency into the Cython extension module.
