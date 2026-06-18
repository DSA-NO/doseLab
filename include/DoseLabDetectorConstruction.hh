#pragma once

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

class DoseLabDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DoseLabDetectorConstruction(const G4String& gdmlFile = "");
    virtual ~DoseLabDetectorConstruction();

    virtual G4VPhysicalVolume* Construct();

  private:
    G4String fGDMLFile;
};