cmake_minimum_required(VERSION 3.31)
project(BunnyEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(Vulkan REQUIRED)

find_package(glfw3 3.4 QUIET)

if(NOT glfw3_FOUND)
    message(STATUS "GLFW not found via find_package, fetching from source...")
    
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(glfw)
    
    add_library(glfw3::glfw3 ALIAS glfw)
else()
    message(STATUS "Found GLFW via find_package")
endif()

find_package(glm QUIET)

if(NOT glm_FOUND)
    message(STATUS "GLM not found via find_package, fetching from source...")
    
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
    )
    
    FetchContent_MakeAvailable(glm)
else()
    message(STATUS "Found GLM via find_package")
endif()

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/app.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Vulkan::Vulkan
    glfw
    glm::glm
)

target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)