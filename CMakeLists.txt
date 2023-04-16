cmake_minimum_required(VERSION 3.22)
project(chat VERSION 1.0 LANGUAGES CXX C)

set(target_name chat)

# put executable in build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)