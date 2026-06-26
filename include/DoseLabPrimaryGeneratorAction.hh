// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabPrimaryGeneratorAction.hh
/// \brief Definition of the DoseLab::DoseLabPrimaryGeneratorAction class

#ifndef DoseLabPrimaryGeneratorAction_h
#define DoseLabPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"

class G4GeneralParticleSource;
class G4Event;

namespace DoseLab
{

/// The primary generator action class with general particle source (GPS).
///
/// It uses G4GeneralParticleSource for flexible particle generation.
/// Particles can be configured via G4 commands in macros or interactively.
/// (see the macros provided with this example).

class DoseLabPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    DoseLabPrimaryGeneratorAction();
    ~DoseLabPrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

  private:
    G4GeneralParticleSource* fParticleSource = nullptr;  // G4 general particle source
};

}  // namespace DoseLab

#endif
