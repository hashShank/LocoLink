cmake_minimum_required(VERSION 3.10)
project(LocoLink)

set(CMAKE_CXX_STANDARD 17)

# Point CMake to the Homebrew installation paths
include_directories(/opt/homebrew/opt/mysql-connector-c++/include)
link_directories(/opt/homebrew/opt/mysql-connector-c++/lib)

add_executable(LocoLink
    src/main.cpp
    src/DatabaseManager.cpp
    src/SystemManager.cpp
    src/RouteEngine.cpp
    src/RouteCache.cpp
)

target_include_directories(LocoLink PRIVATE include)

# Link the modern MySQL X DevAPI library
target_link_libraries(LocoLink mysqlcppconnx)