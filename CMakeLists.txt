#----------------------------------------------------------------------------
# Setup the project
#
cmake_minimum_required(VERSION 3.16...3.27)
project(doseLab)

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
# Use root-config directly for robustness across ROOT/CMake version combinations.
find_program(ROOT_CONFIG_EXECUTABLE root-config)
if(ROOT_CONFIG_EXECUTABLE)
  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --cflags
    OUTPUT_VARIABLE ROOT_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --libs
    OUTPUT_VARIABLE ROOT_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  separate_arguments(ROOT_CFLAGS_LIST NATIVE_COMMAND "${ROOT_CFLAGS}")
  separate_arguments(ROOT_LIBS_LIST NATIVE_COMMAND "${ROOT_LIBS}")

  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/doseLabRootSummary
    COMMAND ${CMAKE_CXX_COMPILER}
            ${PROJECT_SOURCE_DIR}/tools/doseLabRootSummary.cc
            -o ${PROJECT_BINARY_DIR}/doseLabRootSummary
            ${ROOT_CFLAGS_LIST}
            ${ROOT_LIBS_LIST}
    DEPENDS ${PROJECT_SOURCE_DIR}/tools/doseLabRootSummary.cc
    COMMAND_EXPAND_LISTS
    VERBATIM)

  add_custom_target(doseLabRootSummaryTarget ALL
    DEPENDS ${PROJECT_BINARY_DIR}/doseLabRootSummary)

  message(STATUS "ROOT found via root-config: enabled target doseLabRootSummary")
else()
  message(STATUS "ROOT not found (root-config missing): skipping target doseLabRootSummary")
endif()

#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build doseLab. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(DOSELAB_SCRIPTS
  cavity-farmer.mac
  cavity-roos.mac
  exampleB4.in
  field-ref-10x10-ssd100.mac
  gui.mac
  init_vis.mac
  plotHisto.C
  plotNtuple.C
  run-ref-10x10-d5cm-10mv-custom.mac
  run-ref-10x10-d5cm-10mv-farmer.mac
  run-ref-10x10-d5cm-10mv-roos.mac
  run-ref-10x10-d5cm-6mv-custom.mac
  run-ref-10x10-d5cm-6mv-farmer.mac
  run-ref-10x10-d5cm-6mv-roos.mac
  run-ref-10x10-d5cm-co60-custom.mac
  run-ref-10x10-d5cm-co60-farmer.mac
  run-ref-10x10-d5cm-co60-roos.mac
  run-ref-10x10-d10cm-10mv-custom.mac
  run-ref-10x10-d10cm-10mv-farmer.mac
  run-ref-10x10-d10cm-10mv-roos.mac
  run-ref-10x10-d10cm-6mv-custom.mac
  run-ref-10x10-d10cm-6mv-farmer.mac
  run-ref-10x10-d10cm-6mv-roos.mac
  run-ref-10x10-d10cm-co60-custom.mac
  run-ref-10x10-d10cm-co60-farmer.mac
  run-ref-10x10-d10cm-co60-roos.mac
  run-simple.mac
  run-full.mac
  source-co60-teletherapy.mac
  source-linac-photons-10mv.mac
  source-linac-photons-6mv.mac
  source.mac
  vis.mac
  vis-ref-10x10-d5cm-co60-farmer.mac
  vis-ref-10x10-d5cm-co60-roos.mac
  vis-ref-10x10-d5cm-co60-custom.mac
  vis-ref-10x10-d5cm-6mv-farmer.mac
  vis-ref-10x10-d5cm-6mv-roos.mac
  vis-ref-10x10-d5cm-6mv-custom.mac
  vis-ref-10x10-d5cm-10mv-farmer.mac
  vis-ref-10x10-d5cm-10mv-roos.mac
  vis-ref-10x10-d5cm-10mv-custom.mac
  vis-ref-10x10-d10cm-co60-farmer.mac
  vis-ref-10x10-d10cm-co60-roos.mac
  vis-ref-10x10-d10cm-co60-custom.mac
  vis-ref-10x10-d10cm-6mv-farmer.mac
  vis-ref-10x10-d10cm-6mv-roos.mac
  vis-ref-10x10-d10cm-6mv-custom.mac
  vis-ref-10x10-d10cm-10mv-farmer.mac
  vis-ref-10x10-d10cm-10mv-roos.mac
  vis-ref-10x10-d10cm-10mv-custom.mac
  )

foreach(_script ${DOSELAB_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/macros/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/macros
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
