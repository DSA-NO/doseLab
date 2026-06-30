#----------------------------------------------------------------------------
# Setup the project
#
cmake_minimum_required(VERSION 3.16...3.30)
project(doseLab)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# See the documentation for a guide on how to enable/disable specific components
#
find_package(Geant4 REQUIRED ui_all vis_all)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, use our local headers, and link it to the Geant4 libraries
#
add_executable(doseLab doseLab.cc ${sources} ${headers})
target_include_directories(doseLab PRIVATE include)
target_link_libraries(doseLab PRIVATE ${Geant4_LIBRARIES})

# Optional ROOT-based summary helper for post-run output inspection.
option(DOSELAB_BUILD_ROOT_SUMMARY "Build doseLabRootSummary helper when ROOT is available" ON)
option(DOSELAB_REQUIRE_ROOT_SUMMARY "Fail configure if doseLabRootSummary cannot be configured" OFF)
set(DOSELAB_ROOT_CONFIG "" CACHE FILEPATH "Absolute path to root-config for doseLabRootSummary")

if(DOSELAB_BUILD_ROOT_SUMMARY)
  set(_doseLab_root_summary_configured OFF)

  # Use root-config for robust compatibility across ROOT/CMake combinations.
  if(DOSELAB_ROOT_CONFIG)
    set(ROOT_CONFIG_EXECUTABLE "${DOSELAB_ROOT_CONFIG}")
  else()
    set(_doseLab_root_config_hints)
    if(DEFINED ENV{ROOTSYS})
      list(APPEND _doseLab_root_config_hints "$ENV{ROOTSYS}/bin")
    endif()
    if(DEFINED ENV{CONDA_PREFIX})
      list(APPEND _doseLab_root_config_hints "$ENV{CONDA_PREFIX}/bin")
    endif()

    # Common local ROOT locations for users who did not activate their environment.
    file(GLOB _doseLab_mamba_bins "$ENV{HOME}/micromamba/envs/*/bin")
    file(GLOB _doseLab_conda_bins
      "$ENV{HOME}/miniconda3/envs/*/bin"
      "$ENV{HOME}/anaconda3/envs/*/bin")
    list(APPEND _doseLab_root_config_hints ${_doseLab_mamba_bins} ${_doseLab_conda_bins})
    list(REMOVE_DUPLICATES _doseLab_root_config_hints)

    unset(ROOT_CONFIG_EXECUTABLE)
    find_program(
      ROOT_CONFIG_EXECUTABLE
      NAMES root-config
      HINTS
        ${_doseLab_root_config_hints})

    if(ROOT_CONFIG_EXECUTABLE)
      set(DOSELAB_ROOT_CONFIG
          "${ROOT_CONFIG_EXECUTABLE}"
          CACHE FILEPATH "Absolute path to root-config for doseLabRootSummary" FORCE)
    endif()
  endif()

  if(ROOT_CONFIG_EXECUTABLE AND EXISTS "${ROOT_CONFIG_EXECUTABLE}")
    execute_process(
      COMMAND ${ROOT_CONFIG_EXECUTABLE} --cflags
      OUTPUT_VARIABLE ROOT_CFLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE ROOT_CFLAGS_RESULT)
    execute_process(
      COMMAND ${ROOT_CONFIG_EXECUTABLE} --libs
      OUTPUT_VARIABLE ROOT_LIBS
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE ROOT_LIBS_RESULT)

    if(ROOT_CFLAGS_RESULT EQUAL 0 AND ROOT_LIBS_RESULT EQUAL 0)
      separate_arguments(ROOT_CFLAGS_LIST NATIVE_COMMAND "${ROOT_CFLAGS}")
      separate_arguments(ROOT_LIBS_LIST NATIVE_COMMAND "${ROOT_LIBS}")

      add_executable(doseLabRootSummary tools/doseLabRootSummary.cc)
      target_include_directories(doseLabRootSummary PRIVATE include ${Geant4_INCLUDE_DIRS})
      target_compile_options(doseLabRootSummary PRIVATE ${ROOT_CFLAGS_LIST})
      target_link_libraries(doseLabRootSummary PRIVATE ${ROOT_LIBS_LIST})

      set(_doseLab_root_summary_configured ON)
      message(STATUS "ROOT found via root-config (${ROOT_CONFIG_EXECUTABLE}): enabled target doseLabRootSummary")
    endif()
  endif()

  if(NOT _doseLab_root_summary_configured)
    if(DOSELAB_REQUIRE_ROOT_SUMMARY)
      message(FATAL_ERROR
        "ROOT not found: cannot configure doseLabRootSummary. "
        "Ensure root-config is on PATH, or pass -DDOSELAB_ROOT_CONFIG=/absolute/path/to/root-config")
    else()
      message(STATUS "ROOT not found: skipping target doseLabRootSummary")
      message(STATUS "Hint: ensure root-config is on PATH, or set -DDOSELAB_ROOT_CONFIG=/absolute/path/to/root-config")
    endif()
  endif()
endif()

#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build doseLab. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(DOSELAB_REQUIRED_SCRIPTS
  init_vis.mac
  gui.mac
  vis.mac
  source.mac
  source-linac-photons-6mv.mac
  source-linac-photons-10mv.mac
  source-co60-teletherapy.mac
  field-ref-10x10-ssd100.mac
  cavity-farmer.mac
  cavity-roos.mac
  cavity-farmer-walled.mac
  cavity-roos-walled.mac
  run-simple.mac
  run-full.mac
  run-ref-10x10-d5cm-6mv-farmer.mac
  run-ref-10x10-d5cm-6mv-roos.mac
  run-ref-10x10-d5cm-6mv-farmer-walled.mac
  run-ref-10x10-d5cm-6mv-roos-walled.mac
  vis-ref-10x10-d5cm-6mv-farmer.mac
  vis-ref-10x10-d5cm-6mv-roos.mac
  vis-ref-10x10-d5cm-6mv-farmer-walled.mac
  vis-ref-10x10-d5cm-6mv-roos-walled.mac
)

foreach(_required ${DOSELAB_REQUIRED_SCRIPTS})
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/macros/${_required}")
    message(FATAL_ERROR "Missing required macro/script: macros/${_required}")
  endif()
endforeach()

file(GLOB DOSELAB_MAC_FILES RELATIVE ${PROJECT_SOURCE_DIR}/macros ${PROJECT_SOURCE_DIR}/macros/*.mac)
file(GLOB DOSELAB_AUX_FILES RELATIVE ${PROJECT_SOURCE_DIR}/macros ${PROJECT_SOURCE_DIR}/macros/*.in ${PROJECT_SOURCE_DIR}/macros/*.C)
set(DOSELAB_SCRIPTS ${DOSELAB_MAC_FILES} ${DOSELAB_AUX_FILES})
list(REMOVE_DUPLICATES DOSELAB_SCRIPTS)
list(SORT DOSELAB_SCRIPTS)

foreach(_script ${DOSELAB_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/macros/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/macros
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})

install(TARGETS doseLab
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(TARGET doseLabRootSummary)
  install(TARGETS doseLabRootSummary
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
