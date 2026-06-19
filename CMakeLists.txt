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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DoseLabActionInitialization.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DoseLabPrimaryGeneratorAction.cc
)

target_include_directories(doseLab PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(doseLab PRIVATE
    ${Geant4_LIBRARIES}
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/macros
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/macros
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
