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

#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build doseLab. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(DOSELAB_SCRIPTS
  exampleB4.in
  gui.mac
  init_vis.mac
  plotHisto.C
  plotNtuple.C
  run1.mac
  run2.mac
  vis.mac
  )

foreach(_script ${DOSELAB_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/macros
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
