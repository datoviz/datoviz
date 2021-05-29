@echo off
REM build, wheel, testwheel
set arg=%1

REM Building the C library
IF "%arg%"=="build" (
    if not exist "build" mkdir build
    cd build
    cmake .. -G "MinGW Makefiles"
    mingw32-make
    cd ..
)

REM Building the Cython module and the wheel
IF "%arg%"=="wheel" (
    cd bindings\cython
    python setup.py build_ext -i -c mingw32
    python setup.py develop
    python setup.py sdist bdist_wheel
    cd ..\..
)

REM Testing the wheel in a virtual env
IF "%arg%"=="testwheel" (
    if not exist "bindings\cython\dist" mkdir bindings\cython\dist
    cd bindings\cython\dist
    rmdir /s /q venv
    python -m venv venv
    venv\Scripts\python -m pip install --upgrade pip
    venv\Scripts\pip install datoviz*.whl
    venv\Scripts\python -c "from datoviz import canvas, run; canvas().gui_demo(); run(0)"
    cd ..\..\..
)
