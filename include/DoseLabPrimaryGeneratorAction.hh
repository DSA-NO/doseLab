#pragma once

#include <memory>

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4GeneralParticleSource.hh"

class G4Event;

class DoseLabPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    DoseLabPrimaryGeneratorAction() = default;
    ~DoseLabPrimaryGeneratorAction() override = default;

    void GeneratePrimaries(G4Event* event) override;

  private:
    std::unique_ptr<G4GeneralParticleSource> fGPS;
};
