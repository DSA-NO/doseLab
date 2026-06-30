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

/// Run action class
///
/// It accumulates statistic and computes dispersion of the energy deposit
/// and track lengths of charged particles with use of analysis tools:
/// H1D histograms are created in BeginOfRunAction() for the following
/// physics quantities:
/// - Edep in absorber
/// - Edep in gap
/// - Track length in absorber
/// - Track length in gap
/// The same values are also saved in the ntuple.
/// The histograms and ntuple are saved in the output file in a format
/// according to a specified file extension.
///
/// In EndOfRunAction(), the accumulated statistic and computed
/// dispersion is printed.
///

class DoseLabRunAction : public G4UserRunAction
{
  public:
    DoseLabRunAction();
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
