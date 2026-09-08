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
  DoseLabDetectorConstruction* detectorConstruction,
  const G4String& emModel,
  G4bool enableRadioactiveDecay)
: fDetectorConstruction(detectorConstruction)
, fEmModel(emModel)
, fEnableRadioactiveDecay(enableRadioactiveDecay)
{
}

void DoseLabActionInitialization::BuildForMaster() const
{
  // Keep detector wiring on the master action too: scenario commands can be
  // evaluated in master context before worker initialization.
  SetUserAction(new DoseLabRunAction(fDetectorConstruction, fEmModel, fEnableRadioactiveDecay));
}

void DoseLabActionInitialization::Build() const
{
  SetUserAction(new DoseLabPrimaryGeneratorAction);
  // Worker run action mutates detector presets from /doseLab/scenario/*.
  SetUserAction(new DoseLabRunAction(fDetectorConstruction, fEmModel, fEnableRadioactiveDecay));
  SetUserAction(new DoseLabEventAction);
}

}  // namespace DoseLab
