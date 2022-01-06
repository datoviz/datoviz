export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/git/datoviz-distributed/build
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:~/git/datoviz-distributed/build
export FLASK_APP=serve
flask run -p 1234
