// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabEventAction.hh
/// \brief Definition of the DoseLab::DoseLabEventAction class

#ifndef DoseLabEventAction_h
#define DoseLabEventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class G4Event;
template <typename T> class G4THitsMap;

namespace DoseLab
{

/// Event action class
///
/// In EndOfEventAction(), it scores and records the accumulated energy
/// deposit and track length of charged particles in the cavity detector
/// (simulating an ion chamber).

class DoseLabEventAction : public G4UserEventAction
{
  public:
    DoseLabEventAction() = default;
    ~DoseLabEventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

  private:
    // methods
    G4THitsMap<G4double>* GetHitsCollection(G4int hcID, const G4Event* event) const;
    G4double GetSum(G4THitsMap<G4double>* hitsMap) const;
    void PrintEventStatistics(G4double cavityEdep, G4double cavityTrackLength) const;

    // data members
    G4int fCavityEdepHCID = -1;
    G4int fCavityTrackLengthHCID = -1;
};

}  // namespace DoseLab

#endif
