// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabOutputMetadata.hh
/// \brief Shared canonical output metadata definitions.

#ifndef DoseLabOutputMetadata_h
#define DoseLabOutputMetadata_h 1

#include "globals.hh"

#include <array>
#include <cctype>

namespace DoseLab
{
namespace OutputMetadata
{

template <typename Enum>
struct MetadataChoice
{
  Enum value;
  const char* label;
};

enum class SourceKind
{
  kUnspecified,
  kCo60,
  k6MV,
  k10MV,
};

enum class FieldKind
{
  kUnspecified,
  k10x10Ssd100,
};

enum class ChamberKind
{
  kUnspecified,
  kCustom,
  kFarmer,
  kRoos,
};

inline constexpr std::array<MetadataChoice<SourceKind>, 4> kSourceChoices{{
  {SourceKind::kUnspecified, "unspecified"},
  {SourceKind::kCo60, "co60"},
  {SourceKind::k6MV, "6mv"},
  {SourceKind::k10MV, "10mv"},
}};

inline constexpr std::array<MetadataChoice<FieldKind>, 2> kFieldChoices{{
  {FieldKind::kUnspecified, "unspecified"},
  {FieldKind::k10x10Ssd100, "10x10-ssd100"},
}};

inline constexpr std::array<MetadataChoice<ChamberKind>, 4> kChamberChoices{{
  {ChamberKind::kUnspecified, "unspecified"},
  {ChamberKind::kCustom, "custom"},
  {ChamberKind::kFarmer, "farmer"},
  {ChamberKind::kRoos, "roos"},
}};

template <typename Enum, std::size_t N>
inline Enum ParseMetadataChoice(const G4String& label,
                                const std::array<MetadataChoice<Enum>, N>& choices,
                                Enum fallback)
{
  for (const auto& choice : choices) {
    if (label == choice.label) {
      return choice.value;
    }
  }
  return fallback;
}

template <typename Enum, std::size_t N>
inline const char* CanonicalLabel(Enum value, const std::array<MetadataChoice<Enum>, N>& choices,
                                  const char* fallback)
{
  for (const auto& choice : choices) {
    if (choice.value == value) {
      return choice.label;
    }
  }
  return fallback;
}

template <typename Enum, std::size_t N>
inline G4String CandidateLabels(const std::array<MetadataChoice<Enum>, N>& choices)
{
  G4String result;
  bool first = true;
  for (const auto& choice : choices) {
    if (!first) {
      result += " ";
    }
    result += choice.label;
    first = false;
  }
  return result;
}

template <typename Enum, std::size_t N>
inline bool IsCanonicalLabel(const G4String& label,
                             const std::array<MetadataChoice<Enum>, N>& choices)
{
  for (const auto& choice : choices) {
    if (label == choice.label) {
      return true;
    }
  }
  return false;
}

inline SourceKind ParseSourceKind(const G4String& source)
{
  return ParseMetadataChoice(source, kSourceChoices, SourceKind::kUnspecified);
}

inline FieldKind ParseFieldKind(const G4String& field)
{
  return ParseMetadataChoice(field, kFieldChoices, FieldKind::kUnspecified);
}

inline ChamberKind ParseChamberKind(const G4String& chamber)
{
  return ParseMetadataChoice(chamber, kChamberChoices, ChamberKind::kUnspecified);
}

inline const char* CanonicalLabel(SourceKind source)
{
  return CanonicalLabel(source, kSourceChoices, "unspecified");
}

inline const char* CanonicalLabel(FieldKind field)
{
  return CanonicalLabel(field, kFieldChoices, "unspecified");
}

inline const char* CanonicalLabel(ChamberKind chamber)
{
  return CanonicalLabel(chamber, kChamberChoices, "unspecified");
}

inline bool IsValidOutputTagChar(char ch)
{
  const auto uch = static_cast<unsigned char>(ch);
  return std::isalnum(uch) || ch == '-' || ch == '_';
}

inline bool IsValidOutputTag(const G4String& tag)
{
  if (tag.empty()) {
    return false;
  }

  for (auto ch : tag) {
    if (!IsValidOutputTagChar(ch)) {
      return false;
    }
  }
  return true;
}

}  // namespace OutputMetadata
}  // namespace DoseLab

#endif