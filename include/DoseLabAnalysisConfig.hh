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
inline constexpr const char* kCavityTrackLengthCollection = "Cavity/TrackLength";

inline constexpr G4int kDoseH1Id = 0;
inline constexpr G4int kEdepH1Id = 1;
inline constexpr G4int kTrackLengthH1Id = 2;

inline constexpr G4int kDoseNtupleColumn = 0;
inline constexpr G4int kEdepNtupleColumn = 1;
inline constexpr G4int kTrackLengthNtupleColumn = 2;

inline constexpr const char* kDoseH1Name = "Dcav";
inline constexpr const char* kEdepH1Name = "Ecav";
inline constexpr const char* kTrackLengthH1Name = "Lcav";

inline constexpr const char* kCavityNtupleName = "cavity";
inline constexpr const char* kCavityNtupleTitle = "Cavity dose, energy and track length";
inline constexpr const char* kDoseColumnName = "Dose";
inline constexpr const char* kEdepColumnName = "Edep";
inline constexpr const char* kTrackLengthColumnName = "TrackL";

}  // namespace AnalysisConfig
}  // namespace DoseLab

#endif