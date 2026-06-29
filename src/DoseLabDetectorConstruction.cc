// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabDetectorConstruction.cc
/// \brief Implementation of the DoseLab::DoseLabDetectorConstruction class

#include "DoseLabDetectorConstruction.hh"
#include "DoseLabDetectorMessenger.hh"

#include "G4AutoDelete.hh"
#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4GlobalMagFieldMessenger.hh"
#include "G4RotationMatrix.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4NistManager.hh"
#include "G4PSDoseDeposit.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4PSTrackLength.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalConstants.hh"
#include "G4RunManager.hh"
#include "G4SDChargedFilter.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VPrimitiveScorer.hh"
#include "G4VisAttributes.hh"

#include <cctype>

namespace DoseLab
{

G4ThreadLocal G4GlobalMagFieldMessenger* DoseLabDetectorConstruction::fMagFieldMessenger = nullptr;

namespace
{

G4String ToLower(const G4String& value)
{
  G4String lowered = value;
  for (auto& ch : lowered) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return lowered;
}

}  // namespace

DoseLabDetectorConstruction::DoseLabDetectorConstruction()
{
  fCavityRadius = 1. * cm;
  fCavityThickness = 0.5 * cm;
  fCavityDepth = 5. * cm;
  fCavityMaterialName = "G4_AIR";
  fCavityAxis = CavityAxis::kZ;
  fCavityType = "custom";

  fDetectorMessenger = new DoseLabDetectorMessenger(this);
}

DoseLabDetectorConstruction::~DoseLabDetectorConstruction()
{
  delete fDetectorMessenger;
}

void DoseLabDetectorConstruction::SetCavityType(const G4String& type)
{
  ApplyCavityPreset(type);
}

void DoseLabDetectorConstruction::SetCavityRadius(G4double radius)
{
  if (radius <= 0.) {
    return;
  }
  fCavityRadius = radius;
  fCavityType = "custom";
}

void DoseLabDetectorConstruction::SetCavityThickness(G4double thickness)
{
  if (thickness <= 0.) {
    return;
  }
  fCavityThickness = thickness;
  fCavityType = "custom";
}

void DoseLabDetectorConstruction::SetCavityDepth(G4double depth)
{
  if (depth <= 0.) {
    return;
  }
  fCavityDepth = depth;
  fCavityType = "custom";
}

void DoseLabDetectorConstruction::SetCavityAxis(const G4String& axis)
{
  auto lowered = ToLower(axis);
  if (lowered == "x") {
    fCavityAxis = CavityAxis::kX;
  }
  else if (lowered == "y") {
    fCavityAxis = CavityAxis::kY;
  }
  else if (lowered == "z") {
    fCavityAxis = CavityAxis::kZ;
  }
  else {
    return;
  }

  fCavityType = "custom";
}

void DoseLabDetectorConstruction::SetCavityMaterial(const G4String& materialName)
{
  if (materialName.empty()) {
    return;
  }
  fCavityMaterialName = materialName;
  fCavityType = "custom";
}

G4String DoseLabDetectorConstruction::GetCavitySummary() const
{
  G4String axis = "z";
  if (fCavityAxis == CavityAxis::kX) {
    axis = "x";
  }
  else if (fCavityAxis == CavityAxis::kY) {
    axis = "y";
  }

  G4String summary = "Cavity config -> type=" + fCavityType + ", radius="
                     + std::to_string(fCavityRadius / mm)
                     + " mm, thickness=" + std::to_string(fCavityThickness / mm)
                     + " mm, depth=" + std::to_string(fCavityDepth / mm)
                     + " mm, axis=" + axis + ", material=" + fCavityMaterialName;
  return summary;
}

void DoseLabDetectorConstruction::ApplyCavityPreset(const G4String& type)
{
  auto lowered = ToLower(type);
  if (lowered == "farmer") {
    // Farmer-like cylindrical chamber: axis perpendicular to beam.
    fCavityRadius = 0.30 * cm;
    fCavityThickness = 2.30 * cm;
    fCavityAxis = CavityAxis::kX;
    fCavityType = "farmer";
  }
  else if (lowered == "roos") {
    // Roos-like parallel chamber: axis along beam.
    fCavityRadius = 0.80 * cm;
    fCavityThickness = 0.20 * cm;
    fCavityAxis = CavityAxis::kZ;
    fCavityType = "roos";
  }
  else if (lowered == "custom") {
    fCavityType = "custom";
  }
}

G4RotationMatrix* DoseLabDetectorConstruction::BuildCavityRotation() const
{
  if (fCavityAxis == CavityAxis::kZ) {
    return nullptr;
  }

  auto rotation = new G4RotationMatrix();
  if (fCavityAxis == CavityAxis::kX) {
    rotation->rotateY(90. * deg);
  }
  else if (fCavityAxis == CavityAxis::kY) {
    rotation->rotateX(-90. * deg);
  }

  return rotation;
}

G4VPhysicalVolume* DoseLabDetectorConstruction::Construct()
{
  // Define materials
  DefineMaterials();

  // Define volumes
  return DefineVolumes();
}

void DoseLabDetectorConstruction::DefineMaterials()
{
  // Use NIST Manager to get materials
  auto nistManager = G4NistManager::Instance();

  // Air material
  nistManager->FindOrBuildMaterial("G4_AIR");

  // Water material
  nistManager->FindOrBuildMaterial("G4_WATER");

  // User-selected cavity material
  nistManager->FindOrBuildMaterial(fCavityMaterialName);

  // Print materials
  G4cout << *(G4Material::GetMaterialTable()) << G4endl;
}

G4VPhysicalVolume* DoseLabDetectorConstruction::DefineVolumes()
{
  // Geometry parameters
  G4double worldSize = 3. * m;
  G4double phantomSize = 30. * cm;
  G4double phantomCenterZ = -phantomSize / 2.;

  // Get materials
  auto worldMaterial = G4Material::GetMaterial("G4_AIR");
  auto phantomMaterial = G4Material::GetMaterial("G4_WATER");
  auto cavityMaterial = G4Material::GetMaterial(fCavityMaterialName);

  if (!worldMaterial || !phantomMaterial || !cavityMaterial) {
    G4ExceptionDescription msg;
    msg << "Cannot retrieve materials already defined.";
    G4Exception("DoseLabDetectorConstruction::DefineVolumes()", "MyCode0001", FatalException, msg);
  }

  //
  // World
  //
  auto worldS = new G4Box("World",  // its name
                          worldSize / 2, worldSize / 2, worldSize / 2);  // its size

  auto worldLV = new G4LogicalVolume(worldS,  // its solid
                                     worldMaterial,  // its material
                                     "World");  // its name

  auto worldPV = new G4PVPlacement(nullptr,  // no rotation
                                   G4ThreeVector(),  // at (0,0,0)
                                   worldLV,  // its logical volume
                                   "World",  // its name
                                   nullptr,  // its mother volume
                                   false,  // no boolean operation
                                   0,  // copy number
                                   fCheckOverlaps);  // checking overlaps

  //
  // Water Phantom
  //
  auto phantomS = new G4Box("Phantom",  // its name
                            phantomSize / 2, phantomSize / 2, phantomSize / 2);  // its size

  auto phantomLV = new G4LogicalVolume(phantomS,  // its solid
                                       phantomMaterial,  // its material
                                       "Phantom");  // its name

  new G4PVPlacement(nullptr,  // no rotation
                    G4ThreeVector(0., 0., phantomCenterZ),  // centered so phantom top is at z=0
                    phantomLV,  // its logical volume
                    "Phantom",  // its name
                    worldLV,  // its mother volume
                    false,  // no boolean operation
                    0,  // copy number
                    fCheckOverlaps);  // checking overlaps

  //
  // Cavity (Ion Chamber)
  //
  // Position at 5 g/cm^2 depth from phantom entrance surface (z = 0)
  // Phantom top is at local z = +phantomSize/2, so cavity center is at:
  // +phantomSize/2 - cavityDepth relative to phantom center.
  G4double cavityCenterZ = phantomSize / 2 - fCavityDepth;

  auto cavityS = new G4Tubs("Cavity",  // its name
                            0., fCavityRadius, fCavityThickness / 2, 0., twopi);  // rMin, rMax, dz

  auto cavityLV = new G4LogicalVolume(cavityS,  // its solid
                                      cavityMaterial,
                                      "Cavity");  // its name

  new G4PVPlacement(BuildCavityRotation(),  // optional rotation for chamber axis
                    G4ThreeVector(0., 0., cavityCenterZ),  // positioned at depth
                    cavityLV,  // its logical volume
                    "Cavity",  // its name
                    phantomLV,  // its mother volume (inside phantom)
                    false,  // no boolean operation
                    0,  // copy number
                    fCheckOverlaps);  // checking overlaps

  //
  // Print geometry information
  //
  G4cout << G4endl << "------------------------------------------------------------" << G4endl
         << "Geometry Setup:" << G4endl
         << "  World: " << worldSize / cm << " x " << worldSize / cm << " x " << worldSize / cm
         << " cm³ of " << worldMaterial->GetName() << G4endl
      << "  Phantom: " << phantomSize / cm << " x " << phantomSize / cm << " x "
      << phantomSize / cm << " cm³ of " << phantomMaterial->GetName()
      << " (top surface at z = 0 cm)" << G4endl
         << "  Cavity (flat cylinder): diameter " << (2. * fCavityRadius) / cm
         << " cm, thickness " << fCavityThickness / cm << " cm at " << fCavityDepth / cm
         << " cm depth"
      << " (global z = " << (phantomCenterZ + cavityCenterZ) / cm << " cm)" << G4endl
         << "  Cavity material: " << cavityMaterial->GetName() << G4endl
         << "  Cavity preset: " << fCavityType << G4endl
      << "  Source focal point convention: z = +95 cm (SSD=95 cm, SCD=100 cm)" << G4endl
         << "------------------------------------------------------------" << G4endl;

  //
  // Visualization attributes
  //
  worldLV->SetVisAttributes(G4VisAttributes::GetInvisible());

  auto phantomVisAtt = new G4VisAttributes(G4Colour::Blue());
  phantomVisAtt->SetForceSolid(false);
  phantomVisAtt->SetForceWireframe(true);
  phantomLV->SetVisAttributes(phantomVisAtt);

  auto cavityVisAtt = new G4VisAttributes(G4Colour::Red());
  cavityVisAtt->SetForceSolid(true);
  cavityLV->SetVisAttributes(cavityVisAtt);

  //
  // Always return the physical World
  //
  return worldPV;
}

void DoseLabDetectorConstruction::ConstructSDandField()
{
  G4SDManager::GetSDMpointer()->SetVerboseLevel(1);

  //
  // Sensitive Detector for Cavity (Ion Chamber)
  //
  auto cavityDetector = new G4MultiFunctionalDetector("Cavity");
  G4SDManager::GetSDMpointer()->AddNewDetector(cavityDetector);

  G4VPrimitiveScorer* primitive;

  primitive = new G4PSDoseDeposit("Dose");
  cavityDetector->RegisterPrimitive(primitive);

  primitive = new G4PSEnergyDeposit("Edep");
  cavityDetector->RegisterPrimitive(primitive);

  primitive = new G4PSTrackLength("TrackL");
  auto charged = new G4SDChargedFilter("chargedFilter");
  primitive->SetFilter(charged);
  cavityDetector->RegisterPrimitive(primitive);

  SetSensitiveDetector("Cavity", cavityDetector);

  //
  // Magnetic field (optional - set to zero by default)
  //
  G4ThreeVector fieldValue;
  fMagFieldMessenger = new G4GlobalMagFieldMessenger(fieldValue);
  fMagFieldMessenger->SetVerboseLevel(1);

  // Register the field messenger for deleting
  G4AutoDelete::Register(fMagFieldMessenger);
}

}  // namespace DoseLab
