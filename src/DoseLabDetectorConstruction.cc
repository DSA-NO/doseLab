#include "DoseLabDetectorConstruction.hh"

#include "G4Box.hh"
#include "G4GDMLParser.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

DoseLabDetectorConstruction::DoseLabDetectorConstruction(const G4String& gdmlFile)
    : fGDMLFile(gdmlFile)
{
}

DoseLabDetectorConstruction::~DoseLabDetectorConstruction() = default;

G4VPhysicalVolume* DoseLabDetectorConstruction::Construct()
{
    if (!fGDMLFile.empty()) {
        G4cout << "Loading GDML: " << fGDMLFile << G4endl;
        fParser = std::make_unique<G4GDMLParser>();
        fParser->SetOverlapCheck(true);
        fParser->Read(fGDMLFile, false);

        auto* world = fParser->GetWorldVolume();

        if (!world) {
            G4Exception("DoseLabDetectorConstruction",
                        "GDMLReadError",
                        FatalException,
                        "GDML world volume is null.");
        }

        return world;
    }

    auto nist = G4NistManager::Instance();
    auto worldMat = nist->FindOrBuildMaterial("G4_AIR");

    G4double worldSize = 5.0 * m;

    auto solidWorld = new G4Box("World", worldSize, worldSize, worldSize);

    auto logicWorld = new G4LogicalVolume(solidWorld, worldMat, "World");

    auto physWorld = new G4PVPlacement(nullptr, G4ThreeVector(),
                                       logicWorld, "World",
                                       nullptr, false, 0, true);

    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    return physWorld;
}
