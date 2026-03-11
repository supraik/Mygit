cmake_minimum_required(VERSION 3.10)
project(mygit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Client executable
add_executable(mygit 
    src/main.cpp 
    src/commands/init.cpp
    src/commands/add.cpp
    src/commands/rm.cpp
    src/commands/commit.cpp
    src/commands/log.cpp
    src/commands/checkout.cpp
    src/commands/status.cpp
    src/commands/ls-files.cpp
    src/commands/cat-file.cpp
    src/commands/branch.cpp
    src/commands/merge.cpp
    src/commands/push.cpp
    src/commands/pull.cpp
    src/commands/remote.cpp
    src/commands/resolve.cpp
    src/utils/hash.cpp
)

target_include_directories(mygit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link Windows libraries
target_link_libraries(mygit PRIVATE crypt32 ws2_32)

