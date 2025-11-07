cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# module settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SOURCE_FILE_ENCODING UTF-8 NO-BOM)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dacfd01daf")
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
# export for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(arcvia VERSION 1.0.0 LANGUAGES CXX)

# use ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# naming differences
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(EXECUTABLE_NAME "arcvia-debug")
    add_compile_options(-g -O0 -DDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(EXECUTABLE_NAME "arcvia-release") 
    add_compile_options(-O3 -DNDEBUG)
else()
    set(EXECUTABLE_NAME "arcvia")
    add_compile_options(-O3)
endif()

# set output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE SOURCES 
    ${CMAKE_SOURCE_DIR}/src_core/*.cpp
    ${CMAKE_SOURCE_DIR}/src_core/**/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/**/*.cpp
)

file(GLOB_RECURSE MODULE_IFACES CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/module/*.cppm")

file(GLOB_RECURSE MODULE_IMPLS  CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/module/*.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCES})

# includes & modules
target_sources(${EXECUTABLE_NAME}
    PUBLIC FILE_SET CXX_MODULES FILES ${MODULE_IFACES}
    PRIVATE ${MODULE_IMPLS} src_core/main.cpp)

target_include_directories(${EXECUTABLE_NAME} 
    PRIVATE ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/src_core)

set(MSYS2_ROOT "C:/msys64/clang64")

target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${MSYS2_ROOT}/include
    ${MSYS2_ROOT}/include/freetype2
)

# include & linkages

target_link_directories(${EXECUTABLE_NAME} PRIVATE
    ${MSYS2_ROOT}/lib
    ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    openal
    freetype
    fmt
    glfw3
    glew32
    brotlienc
    brotlidec
    brotlicommon
    lua
    user32
    kernel32
    opengl32
    ws2_32
    mswsock
    wsock32
)

# platform args
if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(${EXECUTABLE_NAME} PRIVATE
            -mconsole 
            -Wl,--disable-dynamicbase
            -Wl,--image-base,0x140000000
            -Wl,-rpath,dlls
        )
    else()
        target_link_options(${EXECUTABLE_NAME} PRIVATE
            -mwindows 
            -Wl,--disable-dynamicbase
            -Wl,--image-base,0x140000000
            -Wl,-rpath,dlls
        )
    endif()
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE
        dl
        pthread
    )
    
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(${EXECUTABLE_NAME} PRIVATE
            -s
        )
    endif()
endif()

# copy one to bin/ for running
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${EXECUTABLE_NAME}> ${CMAKE_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:${EXECUTABLE_NAME}>
    COMMENT "Copying ${EXECUTABLE_NAME} to bin directory"
)