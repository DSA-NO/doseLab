#ifndef LabScoreStatistics_h
#define LabScoreStatistics_h 1

#include "G4AnalysisManager.hh"
#include "G4Threading.hh"
#include "G4VAccumulable.hh"
#include "G4SystemOfUnits.hh"

#include <array>
#include <cmath>
#include <limits>

class ScoreStatistics : public G4VAccumulable
{
  public:
    void Add(G4double dose, G4double edep, G4double track)
    {
      ++fCount;
      const std::array<G4double, 3> values{dose, edep, track};
      for (std::size_t i = 0; i < values.size(); ++i) {
        const auto delta = values[i] - fMean[i];
        fMean[i] += delta / fCount;
        fM2[i] += delta * (values[i] - fMean[i]);
      }
    }

    void Merge(const G4VAccumulable& other) override
    {
      const auto& rhs = static_cast<const ScoreStatistics&>(other);
      if (rhs.fCount == 0) return;
      if (fCount == 0) {
        fCount = rhs.fCount;
        fMean = rhs.fMean;
        fM2 = rhs.fM2;
        return;
      }
      const auto total = static_cast<G4double>(fCount) + rhs.fCount;
      for (std::size_t i = 0; i < fMean.size(); ++i) {
        const auto delta = rhs.fMean[i] - fMean[i];
        fM2[i] += rhs.fM2[i] + delta * delta * fCount * rhs.fCount / total;
        fMean[i] += delta * rhs.fCount / total;
      }
      fCount += rhs.fCount;
    }

    void Reset() override { fCount = 0; fMean = {}; fM2 = {}; }
    G4int Count() const { return fCount; }
    G4double Mean(std::size_t i) const { return fMean[i]; }
    G4double M2(std::size_t i) const { return fM2[i]; }
    G4double StdDev(std::size_t i) const
    {
      return fCount > 1 ? std::sqrt(fM2[i] / (fCount - 1))
                        : std::numeric_limits<G4double>::quiet_NaN();
    }

    static G4int CreateNtuple()
    {
      auto* a = G4AnalysisManager::Instance();
      const auto id = a->CreateNtuple("score_statistics", "Per-event score statistics");
      a->CreateNtupleIColumn("ThreadId");
      a->CreateNtupleIColumn("Region");
      a->CreateNtupleSColumn("Quantity");
      a->CreateNtupleIColumn("Events");
      a->CreateNtupleDColumn("Mean");
      a->CreateNtupleDColumn("M2");
      a->CreateNtupleDColumn("StdDev");
      a->CreateNtupleDColumn("StdErr");
      a->FinishNtuple(id);
      return id;
    }

    void WriteRows(G4int id, G4int region) const
    {
      auto* a = G4AnalysisManager::Instance();
      const std::array<const char*, 3> names{"Dose", "Edep", "TrackL"};
      for (std::size_t i = 0; i < names.size(); ++i) {
        const auto sd = StdDev(i);
        a->FillNtupleIColumn(id, 0, G4Threading::G4GetThreadId());
        a->FillNtupleIColumn(id, 1, region);
        a->FillNtupleSColumn(id, 2, names[i]);
        a->FillNtupleIColumn(id, 3, fCount);
        a->FillNtupleDColumn(id, 4, fCount ? fMean[i] : std::numeric_limits<G4double>::quiet_NaN());
        a->FillNtupleDColumn(id, 5, fM2[i]);
        a->FillNtupleDColumn(id, 6, sd);
        a->FillNtupleDColumn(id, 7, fCount > 1 ? sd / std::sqrt(static_cast<G4double>(fCount)) : sd);
        a->AddNtupleRow(id);
      }
    }

    void Print(const G4String& region) const
    {
      const std::array<const char*, 3> names{"Dose (Gy)", "Edep (MeV)", "TrackL (mm)"};
      const std::array<G4double, 3> units{gray, MeV, mm};
      for (std::size_t i = 0; i < names.size(); ++i) {
        G4cout << "Statistics " << region << " " << names[i] << ": n=" << fCount;
        if (fCount > 0) G4cout << " mean=" << fMean[i] / units[i];
        if (fCount > 1) {
          const auto sd = StdDev(i) / units[i];
          G4cout << " stddev=" << sd << " stderr=" << sd / std::sqrt(static_cast<G4double>(fCount));
        }
        else G4cout << " stddev=undefined stderr=undefined";
        G4cout << G4endl;
      }
    }

  private:
    G4int fCount = 0;
    std::array<G4double, 3> fMean{};
    std::array<G4double, 3> fM2{};
};

#endif
