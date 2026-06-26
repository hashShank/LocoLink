cmake_minimum_required(VERSION 3.10)
project(LocoLink)

set(CMAKE_CXX_STANDARD 17)

# Point CMake to the Homebrew installation paths
include_directories(/opt/homebrew/opt/mysql-connector-c++/include)
link_directories(/opt/homebrew/opt/mysql-connector-c++/lib)

add_executable(LocoLink
    main.cpp
    DatabaseManager.cpp
    SystemManager.cpp
    RouteEngine.cpp
    RouteCache.cpp
)

# Link the modern MySQL X DevAPI library
target_link_libraries(LocoLink mysqlcppconnx)