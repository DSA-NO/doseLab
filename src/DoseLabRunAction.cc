// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabRunAction.cc
/// \brief Implementation of the DoseLab::DoseLabRunAction class

#include "DoseLabRunAction.hh"

#include "DoseLabAnalysisConfig.hh"
#include "DoseLabDetectorConstruction.hh"
#include "DoseLabOutputMetadata.hh"

#include "G4AnalysisManager.hh"
#include "G4Exception.hh"
#include "G4GenericMessenger.hh"
#include "G4RunManager.hh"
#include "G4ScoringManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4UnitsTable.hh"
#include "globals.hh"

#include <cctype>

namespace DoseLab
{

DoseLabRunAction::DoseLabRunAction(DoseLabDetectorConstruction* detectorConstruction)
: fDetectorConstruction(detectorConstruction)
{
  ConfigureCommands();

  // Print progress every event (reference macros are low-statistics).
  G4RunManager::GetRunManager()->SetPrintProgress(1);

  // Create analysis manager.
  auto analysisManager = G4AnalysisManager::Instance();

  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  // Ntuple merging is available with ROOT output.

  // Creating histograms for cavity measurements
  analysisManager->CreateH1(AnalysisConfig::kDoseH1Name, "Dose in cavity", 100, 0., 10 * gray);
  analysisManager->CreateH1(
    AnalysisConfig::kEdepH1Name, "Energy deposit in cavity", 100, 0., 10 * MeV);
  analysisManager->CreateH1(
    AnalysisConfig::kTrackLengthH1Name, "Track length in cavity", 100, 0., 10 * cm);

  analysisManager->CreateNtuple(AnalysisConfig::kCavityNtupleName, AnalysisConfig::kCavityNtupleTitle);
  analysisManager->CreateNtupleDColumn(AnalysisConfig::kDoseColumnName);
  analysisManager->CreateNtupleDColumn(AnalysisConfig::kEdepColumnName);
  analysisManager->CreateNtupleDColumn(AnalysisConfig::kTrackLengthColumnName);
  analysisManager->FinishNtuple();

  // Run metadata ntuple (one row per worker in MT mode, one row per run in sequential mode).
  // Store canonical string labels directly so downstream tools do not need to reverse-map codes.
  analysisManager->CreateNtuple(AnalysisConfig::kRunInfoNtupleName,
                                AnalysisConfig::kRunInfoNtupleTitle);
  analysisManager->CreateNtupleSColumn(AnalysisConfig::kRunInfoTagColumnName);
  analysisManager->CreateNtupleSColumn(AnalysisConfig::kRunInfoSourceColumnName);
  analysisManager->CreateNtupleSColumn(AnalysisConfig::kRunInfoFieldColumnName);
  analysisManager->CreateNtupleDColumn(AnalysisConfig::kRunInfoDepthCmColumnName);
  analysisManager->CreateNtupleSColumn(AnalysisConfig::kRunInfoChamberColumnName);
  analysisManager->CreateNtupleIColumn(AnalysisConfig::kRunInfoThreadIdColumnName);
  analysisManager->FinishNtuple(1);
}

void DoseLabRunAction::ConfigureCommands()
{
  fMessenger = std::make_unique<G4GenericMessenger>(this, "/doseLab/output/", "Output metadata control");

  auto& tagCmd = fMessenger->DeclareMethod("tag", &DoseLabRunAction::SetOutputTag,
                                           "Set scenario tag used in output file naming.");
  tagCmd.SetGuidance("Set scenario tag used in output file naming.");
  tagCmd.SetGuidance("Example: /doseLab/output/tag run-ref-10x10-d5cm-6mv-roos");
  tagCmd.SetGuidance("Allowed characters: letters, digits, '-' and '_'.");
  tagCmd.SetParameterName("tag", false);
  tagCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& sourceCmd = fMessenger->DeclareProperty("source", fOutputSource);
  sourceCmd.SetGuidance("Set source metadata string stored in runinfo ntuple.");
  sourceCmd.SetParameterName("source", false);
  sourceCmd.SetCandidates(OutputMetadata::CandidateLabels(OutputMetadata::kSourceChoices));
  sourceCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& fieldCmd = fMessenger->DeclareProperty("field", fOutputField);
  fieldCmd.SetGuidance("Set field metadata string stored in runinfo ntuple.");
  fieldCmd.SetParameterName("field", false);
  fieldCmd.SetCandidates(OutputMetadata::CandidateLabels(OutputMetadata::kFieldChoices));
  fieldCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& depthCmd = fMessenger->DeclareMethodWithUnit(
    "depth", "cm", &DoseLabRunAction::SetOutputDepth, "Set depth metadata stored in runinfo ntuple.");
  depthCmd.SetGuidance("Set depth metadata stored in runinfo ntuple.");
  depthCmd.SetGuidance("Example: /doseLab/output/depth 5 cm");
  depthCmd.SetParameterName("depth", false);
  depthCmd.SetRange("depth>=0.");
  depthCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& chamberCmd = fMessenger->DeclareProperty("chamber", fOutputChamber);
  chamberCmd.SetGuidance("Set chamber metadata string stored in runinfo ntuple.");
  chamberCmd.SetParameterName("chamber", false);
  chamberCmd.SetCandidates(OutputMetadata::CandidateLabels(OutputMetadata::kChamberChoices));
  chamberCmd.SetStates(G4State_PreInit, G4State_Idle);

  fScenarioMessenger =
    std::make_unique<G4GenericMessenger>(this, "/doseLab/scenario/", "Scenario preset control");

  auto& scenarioTypeCmd = fScenarioMessenger->DeclareMethod(
    "type", &DoseLabRunAction::SetScenarioType,
    "Set scenario preset and synchronize cavity geometry with output metadata.");
  scenarioTypeCmd.SetGuidance(
    "Set scenario preset and synchronize cavity geometry with output metadata.");
  scenarioTypeCmd.SetParameterName("scenario", false);
  scenarioTypeCmd.SetCandidates(OutputMetadata::CandidateLabels(OutputMetadata::kScenarioChoices));
  scenarioTypeCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& scenarioDepthCmd = fScenarioMessenger->DeclareMethodWithUnit(
    "depth", "cm", &DoseLabRunAction::SetScenarioDepthOverride,
    "Override scenario depth and synchronize cavity and output metadata depth.");
  scenarioDepthCmd.SetGuidance(
    "Override scenario depth and synchronize cavity and output metadata depth.");
  scenarioDepthCmd.SetGuidance("Example: /doseLab/scenario/depth 5 cm");
  scenarioDepthCmd.SetParameterName("depth", false);
  scenarioDepthCmd.SetRange("depth>=0.");
  scenarioDepthCmd.SetStates(G4State_PreInit, G4State_Idle);
}

void DoseLabRunAction::SetOutputTag(const G4String& tag)
{
  if (!OutputMetadata::IsValidOutputTag(tag)) {
    G4ExceptionDescription msg;
    msg << "Invalid output tag '" << tag
        << "'. Allowed characters are letters, digits, '-' and '_' only.";
    G4Exception("DoseLabRunAction::SetOutputTag", "DoseLabOutput001", FatalException, msg);
    return;
  }

  fOutputTag = tag;
}

void DoseLabRunAction::SetOutputDepth(G4double depth)
{
  fOutputDepthCm = depth / cm;
}

void DoseLabRunAction::SetScenarioType(const G4String& scenario)
{
  fScenarioType = scenario;
  ApplyScenario();
}

void DoseLabRunAction::SetScenarioDepthOverride(G4double depth)
{
  fScenarioDepthOverrideCm = depth / cm;
  ApplyScenario();
}

void DoseLabRunAction::ApplyScenario()
{
  // Scenario commands intentionally synchronize both output metadata and
  // detector cavity presets; users are expected to run these before /run/initialize.
  const auto scenario = OutputMetadata::ParseScenarioKind(fScenarioType);
  const auto* preset = OutputMetadata::FindScenarioPreset(scenario);
  if (!preset) {
    return;
  }

  if (fDetectorConstruction) {
    fDetectorConstruction->SetCavityType(preset->cavityType);
  }

  fOutputChamber = OutputMetadata::CanonicalLabel(preset->chamber);

  G4double depthCm = preset->defaultDepthCm;
  if (fScenarioDepthOverrideCm >= 0.) {
    depthCm = fScenarioDepthOverrideCm;
  }

  if (depthCm >= 0.) {
    fOutputDepthCm = depthCm;
    if (fDetectorConstruction) {
      fDetectorConstruction->SetCavityDepth(depthCm * cm);
    }
  }
}

G4String DoseLabRunAction::SanitizeForFileName(const G4String& value)
{
  G4String out;
  out.reserve(value.size());
  for (auto ch : value) {
    const auto uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) || ch == '-' || ch == '_') {
      out.push_back(ch);
    }
    else {
      out.push_back('_');
    }
  }
  return out;
}

G4String DoseLabRunAction::BuildOutputFileName() const
{
  const auto tag = SanitizeForFileName(this->fOutputTag);
  if (tag.empty()) {
    return "doseLab-default.root";
  }
  return "doseLab-" + tag + ".root";
}

void DoseLabRunAction::BeginOfRunAction(const G4Run* /*run*/)
{
  // Initialize scoring manager for command-based mesh visualization.
  // Enables /score/ commands to create and visualize dose distributions in real-time.
  G4ScoringManager::GetScoringManager();

  // Get analysis manager.
  auto analysisManager = G4AnalysisManager::Instance();

  // Clear previous run data while keeping histogram/ntuple definitions.
  // This makes /vis/plot and /vis/reviewPlots show current-run entries.
  analysisManager->Reset();

  // Open output file.
  G4String fileName = BuildOutputFileName();
  analysisManager->OpenFile(fileName);

  G4cout << "Using " << analysisManager->GetType() << G4endl;
  G4cout << "Output tag: " << fOutputTag << G4endl;
  G4cout << "Output file: " << fileName << G4endl;
}

void DoseLabRunAction::EndOfRunAction(const G4Run* /*run*/)
{
  const auto source = OutputMetadata::ParseSourceKind(fOutputSource);
  const auto field = OutputMetadata::ParseFieldKind(fOutputField);
  const auto chamber = OutputMetadata::ParseChamberKind(fOutputChamber);

  // Add metadata through the worker path in MT mode (same merge path as event ntuple),
  // and directly in sequential mode.
  const auto writeRunInfo = !G4Threading::IsMultithreadedApplication() || !isMaster;

  // print histogram statistics
  //
  auto analysisManager = G4AnalysisManager::Instance();
  if (writeRunInfo) {
    analysisManager->FillNtupleSColumn(1, AnalysisConfig::kRunInfoTagColumn,
                                       SanitizeForFileName(fOutputTag));
    analysisManager->FillNtupleSColumn(1, AnalysisConfig::kRunInfoSourceColumn,
                                       OutputMetadata::CanonicalLabel(source));
    analysisManager->FillNtupleSColumn(1, AnalysisConfig::kRunInfoFieldColumn,
                                       OutputMetadata::CanonicalLabel(field));
    analysisManager->FillNtupleDColumn(1, AnalysisConfig::kRunInfoDepthCmColumn, fOutputDepthCm);
    analysisManager->FillNtupleSColumn(1, AnalysisConfig::kRunInfoChamberColumn,
                                       OutputMetadata::CanonicalLabel(chamber));
    analysisManager->FillNtupleIColumn(1, AnalysisConfig::kRunInfoThreadIdColumn,
                                       G4Threading::G4GetThreadId());
    analysisManager->AddNtupleRow(1);
  }

  if (analysisManager->GetH1(AnalysisConfig::kTrackLengthH1Id)) {
    G4cout << G4endl << " ----> print histograms statistic ";
    if (isMaster) {
      G4cout << "for the entire run " << G4endl << G4endl;
    }
    else {
      G4cout << "for the local thread " << G4endl << G4endl;
    }

    G4cout << " Cavity Dose: mean = "
           << G4BestUnit(analysisManager->GetH1(AnalysisConfig::kDoseH1Id)->mean(), "Dose")
           << " rms = "
           << G4BestUnit(analysisManager->GetH1(AnalysisConfig::kDoseH1Id)->rms(), "Dose")
           << G4endl;

     G4cout << " Cavity Energy Deposit: mean = "
           << G4BestUnit(analysisManager->GetH1(AnalysisConfig::kEdepH1Id)->mean(), "Energy")
           << " rms = "
           << G4BestUnit(analysisManager->GetH1(AnalysisConfig::kEdepH1Id)->rms(), "Energy")
           << G4endl;

     G4cout << " Cavity Track Length: mean = "
           << G4BestUnit(analysisManager->GetH1(AnalysisConfig::kTrackLengthH1Id)->mean(), "Length")
           << " rms = "
           << G4BestUnit(analysisManager->GetH1(AnalysisConfig::kTrackLengthH1Id)->rms(), "Length")
           << G4endl;
  }

  // save histograms & ntuple
  //
  analysisManager->Write();
  // Keep analysis objects in memory for visualization commands in UI session.
  analysisManager->CloseFile(false);
}

}  // namespace DoseLab
