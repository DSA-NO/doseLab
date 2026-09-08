// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabActionInitialization.hh
/// \brief Definition of the DoseLab::DoseLabActionInitialization class

#ifndef DoseLabActionInitialization_h
#define DoseLabActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

#include "globals.hh"

namespace DoseLab
{

/// Action initialization class.

class DoseLabDetectorConstruction;
class DoseLabActionInitialization : public G4VUserActionInitialization
{
  public:
    explicit DoseLabActionInitialization(DoseLabDetectorConstruction* detectorConstruction = nullptr,
      const G4String& emModel = "option4",
      G4bool enableRadioactiveDecay = false);
    ~DoseLabActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;

  private:
    DoseLabDetectorConstruction* fDetectorConstruction = nullptr;
    G4String fEmModel;
    G4bool fEnableRadioactiveDecay = false;
};

}  // namespace DoseLab

#endif
