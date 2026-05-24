cmake_minimum_required(VERSION 3.10)

project(emulator)

add_executable(emu
                main.c
                ./source/cpu.c
)

target_include_directories(emu PRIVATE
                                ${CMAKE_SOURCE_DIR}
                                ${CMAKE_SOURCE_DIR}/include
                                ${CMAKE_SOURCE_DIR}/source
)

target_link_libraries(emu PRIVATE
                                m
)
