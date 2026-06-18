cmake_minimum_required(VERSION 3.16)
project(doseLab VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Geant4 REQUIRED ui_all vis_all)

include(${Geant4_USE_FILE})

add_executable(doseLab
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doseLab.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DoseLabDetectorConstruction.cc
)

target_include_directories(doseLab PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(doseLab PRIVATE
    ${Geant4_LIBRARIES}
)
