#include "DoseLabPrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4GeneralParticleSource.hh"

DoseLabPrimaryGeneratorAction::DoseLabPrimaryGeneratorAction()
    : fGPS(std::make_unique<G4GeneralParticleSource>())
{
}

DoseLabPrimaryGeneratorAction::~DoseLabPrimaryGeneratorAction() = default;

void DoseLabPrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    fGPS->GeneratePrimaryVertex(event);
}