cmake_minimum_required(VERSION 3.16)
project(doseLab VERSION 1.0 LANGUAGES CXX)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Global settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Geant4
find_package(Geant4 REQUIRED COMPONENTS ui_all vis_all gdml)
include(${Geant4_USE_FILE})

# Collect sources
file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cc)

# Executable
add_executable(doseLab
    doseLab.cc
    ${SOURCES}
)

# Include dirs
target_include_directories(doseLab PRIVATE include)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(doseLab PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link Geant4
target_link_libraries(doseLab PRIVATE ${Geant4_LIBRARIES})

# Copy macros
add_custom_command(TARGET doseLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/macros
        $<TARGET_FILE_DIR:doseLab>/macros
)

# Install
include(GNUInstallDirs)

install(TARGETS doseLab
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_SOURCE_DIR}/macros
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
