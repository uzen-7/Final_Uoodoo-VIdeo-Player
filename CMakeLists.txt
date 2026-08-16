cmake_minimum_required(VERSION 3.16)
project(UooDoo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(uoodoo_app
    src/main.cpp
    src/LibraryManager.cpp
)

target_include_directories(uoodoo_app PRIVATE src)

add_executable(uoodoo_qt
    uoodoo_qt.cpp
)
target_link_libraries(uoodoo_qt PRIVATE Qt6::Widgets)

enable_testing()
add_executable(uoodoo_test
    tests/library_manager_test.cpp
    src/LibraryManager.cpp
)
target_include_directories(uoodoo_test PRIVATE src)
add_test(NAME uoodoo_test COMMAND uoodoo_test)
