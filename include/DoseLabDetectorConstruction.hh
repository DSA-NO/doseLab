#pragma once

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

class DoseLabDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DoseLabDetectorConstruction(const G4String& gdmlFile = "");
    ~DoseLabDetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;

  private:
    G4String fGDMLFile;
};