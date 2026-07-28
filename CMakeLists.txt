cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 20)
project(job_system VERSION 0.0.1 LANGUAGES CXX)

file(GLOB SOURCES
    "src/*.cpp" 
    "src/*/*.cpp" )

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)