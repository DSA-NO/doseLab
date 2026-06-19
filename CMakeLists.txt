cmake_minimum_required(VERSION 3.16)
project(doseLab VERSION 1.0 LANGUAGES CXX)

# Global settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Geant4 with requested components
find_package(Geant4 REQUIRED COMPONENTS ui_all vis_all gdml)

# Define executable
add_executable(doseLab
    doseLab.cc
    src/DoseLabDetectorConstruction.cc
    src/DoseLabActionInitialization.cc
    src/DoseLabPrimaryGeneratorAction.cc
)

# Compiler warnings
target_compile_options(doseLab PRIVATE -Wall -Wextra -Wpedantic)

# Include paths (Geant4 headers are handled automatically via target_link_libraries)
target_include_directories(doseLab PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(doseLab PRIVATE
    ${Geant4_LIBRARIES}
)

# Build-time macro copy (updates whenever you run build/make)
add_custom_command(TARGET doseLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/macros
        ${CMAKE_CURRENT_BINARY_DIR}/macros
    COMMENT "Copying simulation macros to build directory..."
)

# Installation rules
include(GNUInstallDirs) # Ensures CMAKE_INSTALL_DATADIR is defined standardly
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/macros
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})