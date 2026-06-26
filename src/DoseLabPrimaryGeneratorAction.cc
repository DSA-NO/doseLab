// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabPrimaryGeneratorAction.cc
/// \brief Implementation of the DoseLab::DoseLabPrimaryGeneratorAction class

#include "DoseLabPrimaryGeneratorAction.hh"

#include "G4GeneralParticleSource.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"

namespace DoseLab
{

DoseLabPrimaryGeneratorAction::DoseLabPrimaryGeneratorAction()
{
  // Create and initialize GeneralParticleSource
  // GPS provides flexible particle generation configurable via UI commands
  fParticleSource = new G4GeneralParticleSource();
}

DoseLabPrimaryGeneratorAction::~DoseLabPrimaryGeneratorAction()
{
  delete fParticleSource;
}

void DoseLabPrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  // GeneratePrimaryVertex is called at the beginning of each event
  // The particle source (type, energy, position, direction) is configured
  // via UI commands or in macros. Default is set by GPS initialization.
  //
  fParticleSource->GeneratePrimaryVertex(event);
}

}  // namespace DoseLab
