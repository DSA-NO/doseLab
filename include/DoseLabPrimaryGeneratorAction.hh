#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"

class G4Event;
class G4GeneralParticleSource;

class DoseLabPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    DoseLabPrimaryGeneratorAction();
    virtual ~DoseLabPrimaryGeneratorAction();

    virtual void GeneratePrimaries(G4Event* event);

  private:
    G4GeneralParticleSource* fGPS;
};