// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabRunAction.hh
/// \brief Definition of the DoseLab::DoseLabRunAction class

#ifndef DoseLabRunAction_h
#define DoseLabRunAction_h 1

#include "G4UserRunAction.hh"

#include "globals.hh"

#include <memory>

class G4Run;
class G4GenericMessenger;

namespace DoseLab
{

class DoseLabDetectorConstruction;

/// Run action for cavity dosimetry output and metadata.
///
/// Creates histograms and ntuples for cavity dose, deposited energy, and
/// charged-particle track length, and writes run-level metadata.
///

class DoseLabRunAction : public G4UserRunAction
{
  public:
    explicit DoseLabRunAction(DoseLabDetectorConstruction* detectorConstruction = nullptr);
    ~DoseLabRunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

  private:
    void ConfigureCommands();
    void SetOutputTag(const G4String& tag);
    void SetOutputDepth(G4double depth);
    void SetScenarioType(const G4String& scenario);
    void SetScenarioDepthOverride(G4double depth);
    void ApplyScenario();
    G4String BuildOutputFileName() const;
    static G4String SanitizeForFileName(const G4String& value);
    DoseLabDetectorConstruction* fDetectorConstruction = nullptr;

    std::unique_ptr<G4GenericMessenger> fMessenger;
    std::unique_ptr<G4GenericMessenger> fScenarioMessenger;

    G4String fOutputTag = "default";
    G4String fOutputSource = "unspecified";
    G4String fOutputField = "unspecified";
    G4double fOutputDepthCm = -1.;
    G4String fOutputChamber = "unspecified";
    G4String fScenarioType = "unspecified";
    G4double fScenarioDepthOverrideCm = -1.;
};

}  // namespace DoseLab

#endif
