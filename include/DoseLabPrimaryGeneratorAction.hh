#pragma once

#include <memory>

#include "G4VUserPrimaryGeneratorAction.hh"

class G4Event;
class G4GeneralParticleSource;

class DoseLabPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    DoseLabPrimaryGeneratorAction();
    ~DoseLabPrimaryGeneratorAction() override = default;

    void GeneratePrimaries(G4Event* event) override;

  private:
    std::unique_ptr<G4GeneralParticleSource> fGPS;
};