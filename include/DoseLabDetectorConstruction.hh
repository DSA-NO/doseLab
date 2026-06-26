// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabDetectorConstruction.hh
/// \brief Definition of the DoseLab::DoseLabDetectorConstruction class

#ifndef DoseLabDetectorConstruction_h
#define DoseLabDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4GlobalMagFieldMessenger;

namespace DoseLab
{

/// Detector construction class to define materials and geometry.
/// Simple water phantom setup with ion chamber cavity.
///
/// Geometry:
/// - World: air (40 x 40 x 40 cm³)
/// - Water phantom: 30 x 30 x 30 cm³
/// - Cavity (ion chamber): at 5 g/cm² depth (~5 cm for water)
///
/// In ConstructSDandField() sensitive detectors are created for the cavity
/// volume to score energy deposition and track length.

class DoseLabDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DoseLabDetectorConstruction() = default;
    ~DoseLabDetectorConstruction() override = default;

  public:
    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

  private:
    // methods
    //
    void DefineMaterials();
    G4VPhysicalVolume* DefineVolumes();

    // data members
    //
    static G4ThreadLocal G4GlobalMagFieldMessenger* fMagFieldMessenger;
    // magnetic field messenger

    G4bool fCheckOverlaps = true;  // option to activate checking of volumes overlaps
};

}  // namespace DoseLab

#endif
