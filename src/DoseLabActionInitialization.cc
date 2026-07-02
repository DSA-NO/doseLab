// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabActionInitialization.cc
/// \brief Implementation of the DoseLab::DoseLabActionInitialization class

#include "DoseLabActionInitialization.hh"

#include "DoseLabEventAction.hh"
#include "DoseLabDetectorConstruction.hh"
#include "DoseLabPrimaryGeneratorAction.hh"
#include "DoseLabRunAction.hh"

namespace DoseLab
{

DoseLabActionInitialization::DoseLabActionInitialization(
  DoseLabDetectorConstruction* detectorConstruction)
: fDetectorConstruction(detectorConstruction)
{
}

void DoseLabActionInitialization::BuildForMaster() const
{
  SetUserAction(new DoseLabRunAction(fDetectorConstruction));
}

void DoseLabActionInitialization::Build() const
{
  SetUserAction(new DoseLabPrimaryGeneratorAction);
  SetUserAction(new DoseLabRunAction(fDetectorConstruction));
  SetUserAction(new DoseLabEventAction);
}

}  // namespace DoseLab
