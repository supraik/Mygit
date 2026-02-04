cmake_minimum_required(VERSION 3.10)
project(mygit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mygit 
    src/main.cpp 
    src/commands/init.cpp
    src/commands/add.cpp
    src/commands/rm.cpp
    src/utils/hash.cpp
)

target_include_directories(mygit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link Windows Crypto API
target_link_libraries(mygit PRIVATE crypt32)
