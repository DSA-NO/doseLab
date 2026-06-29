// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabAnalysisConfig.hh
/// \brief Shared analysis object names and indices.

#ifndef DoseLabAnalysisConfig_h
#define DoseLabAnalysisConfig_h 1

#include "globals.hh"

namespace DoseLab
{
namespace AnalysisConfig
{

inline constexpr const char* kCavityDoseCollection = "Cavity/Dose";
inline constexpr const char* kCavityEdepCollection = "Cavity/Edep";
inline constexpr const char* kCavityTrackLengthCollection = "Cavity/TrackL";

inline constexpr G4int kDoseH1Id = 0;
inline constexpr G4int kEdepH1Id = 1;
inline constexpr G4int kTrackLengthH1Id = 2;

inline constexpr G4int kDoseNtupleColumn = 0;
inline constexpr G4int kEdepNtupleColumn = 1;
inline constexpr G4int kTrackLengthNtupleColumn = 2;

inline constexpr const char* kDoseH1Name = "Dose";
inline constexpr const char* kEdepH1Name = "Edep";
inline constexpr const char* kTrackLengthH1Name = "TrackL";

inline constexpr const char* kCavityNtupleName = "cavity";
inline constexpr const char* kCavityNtupleTitle = "Cavity dose, energy deposit, and track length";
inline constexpr const char* kDoseColumnName = "Dose";
inline constexpr const char* kEdepColumnName = "Edep";
inline constexpr const char* kTrackLengthColumnName = "TrackL";

inline constexpr G4int kRunInfoTagColumn = 0;
inline constexpr G4int kRunInfoSourceColumn = 1;
inline constexpr G4int kRunInfoFieldColumn = 2;
inline constexpr G4int kRunInfoDepthCmColumn = 3;
inline constexpr G4int kRunInfoChamberColumn = 4;
inline constexpr G4int kRunInfoThreadIdColumn = 5;

inline constexpr const char* kRunInfoNtupleName = "runinfo";
inline constexpr const char* kRunInfoNtupleTitle = "Run metadata for scenario provenance";
inline constexpr const char* kRunInfoTagColumnName = "Tag";
inline constexpr const char* kRunInfoSourceColumnName = "Source";
inline constexpr const char* kRunInfoFieldColumnName = "Field";
inline constexpr const char* kRunInfoDepthCmColumnName = "DepthCm";
inline constexpr const char* kRunInfoChamberColumnName = "Chamber";
inline constexpr const char* kRunInfoThreadIdColumnName = "ThreadId";

}  // namespace AnalysisConfig
}  // namespace DoseLab

#endif