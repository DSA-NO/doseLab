// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabRunAction.cc
/// \brief Implementation of the DoseLab::DoseLabRunAction class

#include "DoseLabRunAction.hh"

#include "DoseLabAnalysisConfig.hh"

#include "G4AnalysisManager.hh"
#include "G4GenericMessenger.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "globals.hh"

#include <cctype>

namespace DoseLab
{

DoseLabRunAction::DoseLabRunAction()
{
  ConfigureCommands();

  // set printing event number per each event
  G4RunManager::GetRunManager()->SetPrintProgress(1);

  // Create analysis manager
  // The choice of the output format is done via the specified
  // file extension.
  auto analysisManager = G4AnalysisManager::Instance();

  // Create directories
  // analysisManager->SetHistoDirectoryName("histograms");
  // analysisManager->SetNtupleDirectoryName("ntuple");
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  // Note: merging ntuples is available only with Root output

  // Book histograms, ntuple
  //

  // Creating histograms for cavity measurements
  analysisManager->CreateH1(AnalysisConfig::kDoseH1Name, "Dose in cavity", 100, 0., 10 * gray);
  analysisManager->CreateH1(AnalysisConfig::kEdepH1Name, "Edep in cavity", 100, 0., 10 * MeV);
  analysisManager->CreateH1(
    AnalysisConfig::kTrackLengthH1Name, "TrackL in cavity", 100, 0., 10 * cm);

  // Creating ntuple
  //
  analysisManager->CreateNtuple(AnalysisConfig::kCavityNtupleName, AnalysisConfig::kCavityNtupleTitle);
  analysisManager->CreateNtupleDColumn(AnalysisConfig::kDoseColumnName);
  analysisManager->CreateNtupleDColumn(AnalysisConfig::kEdepColumnName);
  analysisManager->CreateNtupleDColumn(AnalysisConfig::kTrackLengthColumnName);
  analysisManager->FinishNtuple();

  // Run metadata ntuple (one row per run)
  analysisManager->CreateNtuple("runinfo", "Run metadata for scenario provenance");
  analysisManager->CreateNtupleSColumn("Tag");
  analysisManager->CreateNtupleSColumn("Source");
  analysisManager->CreateNtupleSColumn("Field");
  analysisManager->CreateNtupleSColumn("Depth");
  analysisManager->CreateNtupleSColumn("Chamber");
  analysisManager->FinishNtuple(1);
}

void DoseLabRunAction::ConfigureCommands()
{
  fMessenger = std::make_unique<G4GenericMessenger>(this, "/doseLab/output/", "Output metadata control");

  auto& tagCmd = fMessenger->DeclareProperty("tag", fOutputTag);
  tagCmd.SetGuidance("Set scenario tag used in output file naming.");
  tagCmd.SetGuidance("Example: /doseLab/output/tag run-ref-10x10-d5cm-6mv-roos");
  tagCmd.SetParameterName("tag", false);
  tagCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& sourceCmd = fMessenger->DeclareProperty("source", fOutputSource);
  sourceCmd.SetGuidance("Set source metadata string stored in runinfo ntuple.");
  sourceCmd.SetParameterName("source", false);
  sourceCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& fieldCmd = fMessenger->DeclareProperty("field", fOutputField);
  fieldCmd.SetGuidance("Set field metadata string stored in runinfo ntuple.");
  fieldCmd.SetParameterName("field", false);
  fieldCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& depthCmd = fMessenger->DeclareProperty("depth", fOutputDepth);
  depthCmd.SetGuidance("Set depth metadata string stored in runinfo ntuple.");
  depthCmd.SetParameterName("depth", false);
  depthCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto& chamberCmd = fMessenger->DeclareProperty("chamber", fOutputChamber);
  chamberCmd.SetGuidance("Set chamber metadata string stored in runinfo ntuple.");
  chamberCmd.SetParameterName("chamber", false);
  chamberCmd.SetStates(G4State_PreInit, G4State_Idle);
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
  // inform the runManager to save random number seed
  // G4RunManager::GetRunManager()->SetRandomNumberStore(true);

  // Get analysis manager
  auto analysisManager = G4AnalysisManager::Instance();

  // Clear previous run data while keeping histogram/ntuple definitions.
  // This makes /vis/plot and /vis/reviewPlots show current-run entries.
  analysisManager->Reset();

  // Open an output file
  //
  G4String fileName = BuildOutputFileName();
  // Other supported output types:
  // G4String fileName = "B4.csv";
  // G4String fileName = "B4.hdf5";
  // G4String fileName = "B4.xml";
  analysisManager->OpenFile(fileName);
  if (isMaster) {
    analysisManager->FillNtupleSColumn(1, 0, fOutputTag);
    analysisManager->FillNtupleSColumn(1, 1, fOutputSource);
    analysisManager->FillNtupleSColumn(1, 2, fOutputField);
    analysisManager->FillNtupleSColumn(1, 3, fOutputDepth);
    analysisManager->FillNtupleSColumn(1, 4, fOutputChamber);
    analysisManager->AddNtupleRow(1);
  }

  G4cout << "Using " << analysisManager->GetType() << G4endl;
  G4cout << "Output tag: " << fOutputTag << G4endl;
  G4cout << "Output file: " << fileName << G4endl;
}

void DoseLabRunAction::EndOfRunAction(const G4Run* /*run*/)
{
  // print histogram statistics
  //
  auto analysisManager = G4AnalysisManager::Instance();
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

    G4cout << " Cavity Energy: mean = "
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
