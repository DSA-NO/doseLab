#pragma once

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;

class DoseLabDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    explicit DoseLabDetectorConstruction(const G4String& gdmlFile = "");
    ~DoseLabDetectorConstruction() override = default;

    G4VPhysicalVolume* Construct() override;

    const G4String& GetGDMLFile() const { return fGDMLFile; }

  private:
    const G4String fGDMLFile;
};
