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
#include "G4Threading.hh"
#include "G4UnitsTable.hh"
#include "globals.hh"

#include <cctype>
#include <functional>

namespace DoseLab
{

namespace
{

G4int SourceCode(const G4String& source)
{
  if (source == "co60") {
    return 1;
  }
  if (source == "6mv") {
    return 2;
  }
  if (source == "10mv") {
    return 3;
  }
  return 0;
}

G4int FieldCode(const G4String& field)
{
  if (field == "10x10-ssd100") {
    return 1;
  }
  return 0;
}

G4int ChamberCode(const G4String& chamber)
{
  if (chamber == "custom") {
    return 1;
  }
  if (chamber == "farmer") {
    return 2;
  }
  if (chamber == "roos") {
    return 3;
  }
  return 0;
}

G4double ParseDepthCm(const G4String& depth)
{
  // Expected forms: d5cm, d10cm
  if (depth.size() >= 4 && depth[0] == 'd' && depth.substr(depth.size() - 2) == "cm") {
    const auto numberPart = depth.substr(1, depth.size() - 3);
    try {
      return std::stod(numberPart);
    }
    catch (...) {
      return 0.;
    }
  }
  return 0.;
}

G4int TagHash(const G4String& tag)
{
  const auto value = std::hash<std::string>{}(tag);
  return static_cast<G4int>(value & 0x7fffffffU);
}

}  // namespace

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
  // Numeric coding is robust with MT ntuple merging.
  // SourceCode: 1=co60, 2=6mv, 3=10mv
  // FieldCode:  1=10x10-ssd100
  // ChamberCode: 1=custom, 2=farmer, 3=roos
  analysisManager->CreateNtuple("runinfo", "Run metadata for scenario provenance");
  analysisManager->CreateNtupleIColumn("TagHash");
  analysisManager->CreateNtupleIColumn("SourceCode");
  analysisManager->CreateNtupleIColumn("FieldCode");
  analysisManager->CreateNtupleDColumn("DepthCm");
  analysisManager->CreateNtupleIColumn("ChamberCode");
  analysisManager->CreateNtupleIColumn("ThreadId");
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

  G4cout << "Using " << analysisManager->GetType() << G4endl;
  G4cout << "Output tag: " << fOutputTag << G4endl;
  G4cout << "Output file: " << fileName << G4endl;
}

void DoseLabRunAction::EndOfRunAction(const G4Run* /*run*/)
{
  // Add metadata through the worker path in MT mode (same merge path as event ntuple),
  // and directly in sequential mode.
  const auto writeRunInfo = !G4Threading::IsMultithreadedApplication() || !isMaster;

  // print histogram statistics
  //
  auto analysisManager = G4AnalysisManager::Instance();
  if (writeRunInfo) {
    analysisManager->FillNtupleIColumn(1, 0, TagHash(fOutputTag));
    analysisManager->FillNtupleIColumn(1, 1, SourceCode(fOutputSource));
    analysisManager->FillNtupleIColumn(1, 2, FieldCode(fOutputField));
    analysisManager->FillNtupleDColumn(1, 3, ParseDepthCm(fOutputDepth));
    analysisManager->FillNtupleIColumn(1, 4, ChamberCode(fOutputChamber));
    analysisManager->FillNtupleIColumn(1, 5, G4Threading::G4GetThreadId());
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
