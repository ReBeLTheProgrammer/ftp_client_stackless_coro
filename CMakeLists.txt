cmake_minimum_required(VERSION 3.20)
project(ftp_client_coro)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

find_package(Boost 1.75 REQUIRED COMPONENTS program_options)

target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost Boost::program_options)
