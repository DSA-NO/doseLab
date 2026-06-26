// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabRunAction.cc
/// \brief Implementation of the DoseLab::DoseLabRunAction class

#include "DoseLabRunAction.hh"

#include "DoseLabAnalysisConfig.hh"

#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "globals.hh"

namespace DoseLab
{

DoseLabRunAction::DoseLabRunAction()
{
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
  G4String fileName = "B4.root";
  // Other supported output types:
  // G4String fileName = "B4.csv";
  // G4String fileName = "B4.hdf5";
  // G4String fileName = "B4.xml";
  analysisManager->OpenFile(fileName);
  G4cout << "Using " << analysisManager->GetType() << G4endl;
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
