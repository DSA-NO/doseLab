#include "DoseLabDetectorConstruction.hh"

#include "G4Box.hh"
#include "G4GDMLParser.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

DoseLabDetectorConstruction::DoseLabDetectorConstruction(const G4String& gdmlFile)
    : G4VUserDetectorConstruction(),
      fGDMLFile(gdmlFile)
{
}

DoseLabDetectorConstruction::~DoseLabDetectorConstruction() {}

G4VPhysicalVolume* DoseLabDetectorConstruction::Construct()
{
    // -------------------------------------------------
    // Option 1: Load GDML geometry
    // -------------------------------------------------
    if (!fGDMLFile.empty()) {
        G4GDMLParser parser;
        parser.Read(fGDMLFile);
        return parser.GetWorldVolume();
    }

    // -------------------------------------------------
    // Option 2: Minimal fallback geometry (world only)
    // -------------------------------------------------
    auto nist = G4NistManager::Instance();
    auto worldMat = nist->FindOrBuildMaterial("G4_AIR");

    G4double worldSize = 1.0 * m;

    auto solidWorld = new G4Box("World", worldSize, worldSize, worldSize);

    auto logicWorld = new G4LogicalVolume(
        solidWorld,
        worldMat,
        "World");

    auto physWorld = new G4PVPlacement(
        nullptr,
        G4ThreeVector(),
        logicWorld,
        "World",
        nullptr,
        false,
        0,
        true);

    return physWorld;
}