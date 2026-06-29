// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabEventAction.cc
/// \brief Implementation of the DoseLab::DoseLabEventAction class

#include "DoseLabEventAction.hh"

#include "DoseLabAnalysisConfig.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4THitsMap.hh"
#include "G4UnitsTable.hh"

#include <iomanip>

namespace DoseLab
{

G4THitsMap<G4double>* DoseLabEventAction::GetHitsCollection(G4int hcID, const G4Event* event) const
{
  auto hitsCollection = static_cast<G4THitsMap<G4double>*>(event->GetHCofThisEvent()->GetHC(hcID));

  if (!hitsCollection) {
    G4ExceptionDescription msg;
    msg << "Cannot access hitsCollection ID " << hcID;
    G4Exception("DoseLabEventAction::GetHitsCollection()", "MyCode0003", FatalException, msg);
  }

  return hitsCollection;
}

G4double DoseLabEventAction::GetSum(G4THitsMap<G4double>* hitsMap) const
{
  G4double sumValue = 0.;
  for (auto it : *hitsMap->GetMap()) {
    // hitsMap->GetMap() returns the map of std::map<G4int, G4double*>
    sumValue += *(it.second);
  }
  return sumValue;
}

void DoseLabEventAction::PrintEventStatistics(G4double cavityDose, G4double cavityEdep,
                                              G4double cavityTrackLength) const
{
  // Print event statistics
  //
  G4cout << "   Cavity: dose: " << std::setw(7) << G4BestUnit(cavityDose, "Dose")
      << "       energy deposit: " << std::setw(7) << G4BestUnit(cavityEdep, "Energy")
         << "       total track length: " << std::setw(7) << G4BestUnit(cavityTrackLength, "Length")
         << G4endl;
}

void DoseLabEventAction::BeginOfEventAction(const G4Event* /*event*/) {}

void DoseLabEventAction::EndOfEventAction(const G4Event* event)
{
  // Get hits collection IDs for cavity
  if (fCavityDoseHCID == -1) {
    fCavityDoseHCID =
      G4SDManager::GetSDMpointer()->GetCollectionID(AnalysisConfig::kCavityDoseCollection);
    fCavityEdepHCID =
      G4SDManager::GetSDMpointer()->GetCollectionID(AnalysisConfig::kCavityEdepCollection);
    fCavityTrackLengthHCID = G4SDManager::GetSDMpointer()->GetCollectionID(
      AnalysisConfig::kCavityTrackLengthCollection);
  }

  // Get sum values from hits collections
  //
  auto cavityDose = GetSum(GetHitsCollection(fCavityDoseHCID, event));
  auto cavityEdep = GetSum(GetHitsCollection(fCavityEdepHCID, event));
  auto cavityTrackLength = GetSum(GetHitsCollection(fCavityTrackLengthHCID, event));

  // get analysis manager
  auto analysisManager = G4AnalysisManager::Instance();

  // fill histograms
  //
  analysisManager->FillH1(AnalysisConfig::kDoseH1Id, cavityDose);
  analysisManager->FillH1(AnalysisConfig::kEdepH1Id, cavityEdep);
  analysisManager->FillH1(AnalysisConfig::kTrackLengthH1Id, cavityTrackLength);

  // fill ntuple
  //
  analysisManager->FillNtupleDColumn(AnalysisConfig::kDoseNtupleColumn, cavityDose);
  analysisManager->FillNtupleDColumn(AnalysisConfig::kEdepNtupleColumn, cavityEdep);
  analysisManager->FillNtupleDColumn(AnalysisConfig::kTrackLengthNtupleColumn, cavityTrackLength);
  analysisManager->AddNtupleRow();

  // print per event (modulo n)
  //
  auto eventID = event->GetEventID();
  auto printModulo = G4RunManager::GetRunManager()->GetPrintProgress();
  if ((printModulo > 0) && (eventID % printModulo == 0)) {
    PrintEventStatistics(cavityDose, cavityEdep, cavityTrackLength);
    G4cout << "--> End of event: " << eventID << "\n" << G4endl;
  }
}

}  // namespace DoseLab
