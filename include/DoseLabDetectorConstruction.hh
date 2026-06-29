// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabDetectorConstruction.hh
/// \brief Definition of the DoseLab::DoseLabDetectorConstruction class

#ifndef DoseLabDetectorConstruction_h
#define DoseLabDetectorConstruction_h 1

#include "G4RotationMatrix.hh"
#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4GlobalMagFieldMessenger;

namespace DoseLab
{

class DoseLabDetectorMessenger;

/// Detector construction class to define materials and geometry.
/// Water phantom setup with configurable cylindrical ion-chamber cavity.
///
/// Geometry:
/// - World: air (3 x 3 x 3 m³)
/// - Water phantom: 30 x 30 x 30 cm³
/// - Cavity (ion chamber): configurable radius, thickness, depth, axis, material
/// - Simple cavity wall: configurable material and thickness
///
/// Macro interface:
/// - /doseLab/cavity/type farmer|roos|farmer_walled|roos_walled|custom
/// - /doseLab/cavity/radius <value> <unit>
/// - /doseLab/cavity/thickness <value> <unit>
/// - /doseLab/cavity/depth <value> <unit>
/// - /doseLab/cavity/axis x|y|z
/// - /doseLab/cavity/material <G4_NIST_name>
/// - /doseLab/cavity/wallThickness <value> <unit>
/// - /doseLab/cavity/wallMaterial <G4_NIST_name>
/// - /doseLab/cavity/cut <value> <unit>
/// - /doseLab/cavity/wallCut <value> <unit>
/// - /doseLab/cavity/maxStep <value> <unit>
/// - /doseLab/cavity/wallMaxStep <value> <unit>
/// - /doseLab/cavity/update
/// - /doseLab/cavity/print
///
/// In ConstructSDandField() sensitive detectors are created for the cavity
/// volume to score energy deposition and track length.

class DoseLabDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DoseLabDetectorConstruction();
    ~DoseLabDetectorConstruction() override;

  public:
    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    void SetCavityType(const G4String& type);
    void SetCavityRadius(G4double radius);
    void SetCavityThickness(G4double thickness);
    void SetCavityDepth(G4double depth);
    void SetCavityAxis(const G4String& axis);
    void SetCavityMaterial(const G4String& materialName);
    void SetCavityWallThickness(G4double thickness);
    void SetCavityWallMaterial(const G4String& materialName);
    void SetCavityRegionCut(G4double cut);
    void SetWallRegionCut(G4double cut);
    void SetCavityMaxStep(G4double maxStep);
    void SetWallMaxStep(G4double maxStep);

    G4String GetCavitySummary() const;

  private:
    enum class CavityAxis
    {
      kX,
      kY,
      kZ
    };

    // methods
    void DefineMaterials();
    G4VPhysicalVolume* DefineVolumes();
    void ApplyCavityPreset(const G4String& type);
    G4RotationMatrix* BuildCavityRotation() const;

    // data members
    static G4ThreadLocal G4GlobalMagFieldMessenger* fMagFieldMessenger;
    DoseLabDetectorMessenger* fDetectorMessenger = nullptr;

    G4bool fCheckOverlaps = true;  // option to activate checking of volumes overlaps

    // Macro-configurable cavity parameters
    G4double fCavityRadius = 0.;
    G4double fCavityThickness = 0.;
    G4double fCavityDepth = 0.;  // from phantom entrance surface (z=0)
    G4String fCavityMaterialName = "G4_AIR";
    G4double fCavityWallThickness = 0.;
    G4String fCavityWallMaterialName = "G4_POLYETHYLENE";

    // Region-specific transport settings for small-cavity dosimetry
    G4double fCavityRegionCut = 0.;
    G4double fWallRegionCut = 0.;
    G4double fCavityMaxStep = 0.;
    G4double fWallMaxStep = 0.;

    CavityAxis fCavityAxis = CavityAxis::kZ;
    G4String fCavityType = "custom";
};

}  // namespace DoseLab

#endif
