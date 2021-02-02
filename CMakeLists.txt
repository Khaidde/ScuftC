cmake_minimum_required(VERSION 3.0.0)
set (CMAKE_CXX_STANDARD 17)

project(
  ScuftC
  VERSION 1.0
  LANGUAGES CXX)

include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(src SOURCES)
add_executable(scft ${SOURCES})
