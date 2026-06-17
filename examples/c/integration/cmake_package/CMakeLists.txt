cmake_minimum_required(VERSION 3.21)
project(datoviz_cmake_package_example C)

find_package(datoviz REQUIRED)

add_executable(datoviz_cmake_package_example main.c)
target_link_libraries(datoviz_cmake_package_example PRIVATE datoviz::datoviz)
