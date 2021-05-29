cd bindings\cython
python setup.py build_ext -i -c mingw32
python setup.py develop
python setup.py sdist bdist_wheel
cd ..\..
