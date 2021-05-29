cd bindings\cython\dist
rmdir /s /q venv
python -m venv venv
venv\Scripts\python -m pip install --upgrade pip
venv\Scripts\pip install datoviz-0.1.0a0-cp38-cp38-win_amd64.whl
venv\Scripts\python -c "import datoviz"
cd ..\..\..
