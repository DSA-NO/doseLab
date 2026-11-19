#ifndef DoseLabScoreStatistics_h
#define DoseLabScoreStatistics_h 1

#include "G4VAccumulable.hh"

#include <array>

namespace DoseLab
{
class DoseLabScoreStatistics : public G4VAccumulable
{
  public:
    void Add(G4double dose, G4double edep, G4double track);
    void Merge(const G4VAccumulable& other) override;
    void Reset() override;
    G4int Count() const;
    G4double Mean(std::size_t i) const;
    G4double M2(std::size_t i) const;
    G4double StdDev(std::size_t i) const;
    static G4int CreateNtuple();
    void WriteRows(G4int id, G4int region) const;
    void Print(const G4String& region) const;

  private:
    G4int fCount = 0;
    std::array<G4double, 3> fMean{};
    std::array<G4double, 3> fM2{};
};
}  // namespace DoseLab

#endif
