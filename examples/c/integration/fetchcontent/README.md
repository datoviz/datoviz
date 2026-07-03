# FetchContent Integration

Minimal C consumer that builds Datoviz as a CMake subproject.

```sh
cmake -S . -B build
cmake --build build
./build/datoviz_fetchcontent_example
```

The default remote ref is `v0.4-dev` while v0.4 release candidates are being prepared. Override
`DATOVIZ_FETCHCONTENT_TAG` once a final release tag is available.

For local Datoviz development, avoid fetching from Git and point the example at a checkout:

```sh
cmake -S . -B build -DDATOVIZ_FETCHCONTENT_SOURCE_DIR=/path/to/datoviz
cmake --build build
```

Datoviz tests, examples, and install/package export rules are disabled by default when Datoviz is
embedded this way.
