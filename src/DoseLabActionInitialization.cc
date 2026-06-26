// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabActionInitialization.cc
/// \brief Implementation of the DoseLab::DoseLabActionInitialization class

#include "DoseLabActionInitialization.hh"

#include "DoseLabEventAction.hh"
#include "DoseLabPrimaryGeneratorAction.hh"
#include "DoseLabRunAction.hh"

namespace DoseLab
{

void DoseLabActionInitialization::BuildForMaster() const
{
  SetUserAction(new DoseLabRunAction);
}

void DoseLabActionInitialization::Build() const
{
  SetUserAction(new DoseLabPrimaryGeneratorAction);
  SetUserAction(new DoseLabRunAction);
  SetUserAction(new DoseLabEventAction);
}

}  // namespace DoseLab
