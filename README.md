# Datoviz v0.2x

Upcoming version of Datoviz, an open-source high-performance interactive scientific data visualization C/C++ library.

## Build

Ubuntu 24.04 notes:

```bash
sudo apt install build-essential cmake gcc ninja-build xorg-dev clang-format libtinyxml2-dev libfreetype-dev
git clone https://github.com/datoviz/datoviz.git@v0.2x
cd datoviz
git submodule update --init
./manage.sh build # there will be an error, you need to call it a second time, fix welcome
                  # see https://github.com/Chlumsky/msdf-atlas-gen/issues/98
./manage.sh build # that one should suceed
```

