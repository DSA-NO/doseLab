#include "DoseLabPrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4GeneralParticleSource.hh"

DoseLabPrimaryGeneratorAction::DoseLabPrimaryGeneratorAction()
    : fGPS(new G4GeneralParticleSource())
{
}

DoseLabPrimaryGeneratorAction::~DoseLabPrimaryGeneratorAction()
{
    delete fGPS;
}

void DoseLabPrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    fGPS->GeneratePrimaryVertex(event);
}