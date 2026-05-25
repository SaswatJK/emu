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

target_compile_options(emu PRIVATE
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Debug>:-O0 -g>
)

target_compile_definitions(emu PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Debug>:DEBUG>
)
