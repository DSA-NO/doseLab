// doseLab - ROOT summary helper
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no

#include "TBranch.h"
#include "TCollection.h"
#include "TFile.h"
#include "TKey.h"
#include "TLeafC.h"
#include "TTree.h"

#include "DoseLabAnalysisConfig.hh"
#include "DoseLabOutputMetadata.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>

namespace
{

struct UnitSpec
{
  double scale;
  const char* label;
};

struct NumericSummary
{
  Long64_t count = 0;
  double mean = 0.;
  double min = 0.;
  double max = 0.;
};

struct RunInfoRow
{
  std::string tag;
  std::string source;
  std::string field;
  Double_t depthCm = 0.;
  std::string chamber;
  Int_t threadId = 0;
};

struct RunInfoSummary
{
  Long64_t rows = 0;
  RunInfoRow referenceRow;
  bool hasReferenceRow = false;
  bool metadataConsistent = true;
  std::set<Int_t> threadIds;
};

std::string FormatScaledValue(double scaledValue)
{
  std::ostringstream out;
  const auto magnitude = std::fabs(scaledValue);
  if (magnitude > 0. && (magnitude < 1.e-3 || magnitude >= 1.e6)) {
    out << std::scientific << std::setprecision(6) << scaledValue;
  }
  else {
    out << std::fixed << std::setprecision(6) << scaledValue;
  }
  return out.str();
}

std::string FormatWithUnits(double value, std::initializer_list<UnitSpec> units)
{
  const auto magnitude = std::fabs(value);
  const UnitSpec* chosenUnit = nullptr;

  for (auto it = units.begin(); it != units.end(); ++it) {
    chosenUnit = &(*it);
  }

  for (const auto& unit : units) {
    if (magnitude >= unit.scale) {
      chosenUnit = &unit;
      break;
    }
  }

  if (!chosenUnit) {
    return FormatScaledValue(value);
  }

  const auto scaledValue = value / chosenUnit->scale;
  return FormatScaledValue(scaledValue) + " " + chosenUnit->label;
}

std::string FormatDose(double value)
{
  constexpr double nanogray = 1.e-9 * gray;
  constexpr double picogray = 1.e-12 * gray;
  constexpr double femtogray = 1.e-15 * gray;
  return FormatWithUnits(value,
                         {{gray, "Gy"},
                          {CLHEP::milligray, "mGy"},
                          {CLHEP::microgray, "uGy"},
                          {nanogray, "nGy"},
                          {picogray, "pGy"},
                          {femtogray, "fGy"}});
}

std::string FormatEnergy(double value)
{
  return FormatWithUnits(value,
                         {{GeV, "GeV"}, {MeV, "MeV"}, {keV, "keV"}, {eV, "eV"}});
}

std::string FormatLength(double value)
{
  return FormatWithUnits(value,
                         {{m, "m"}, {cm, "cm"}, {mm, "mm"}, {um, "um"}, {nm, "nm"}});
}

std::string FormatThreadIdSet(const std::set<Int_t>& threadIds)
{
  std::ostringstream out;
  bool first = true;
  for (const auto threadId : threadIds) {
    if (!first) {
      out << ", ";
    }
    out << threadId;
    first = false;
  }
  return out.str();
}

bool SameScenarioMetadata(const RunInfoRow& left, const RunInfoRow& right)
{
  return left.tag == right.tag && left.source == right.source && left.field == right.field
         && left.depthCm == right.depthCm && left.chamber == right.chamber;
}

bool HasCanonicalMetadataLabels(const RunInfoRow& row)
{
  return DoseLab::OutputMetadata::IsCanonicalLabel(G4String(row.source),
                                                   DoseLab::OutputMetadata::kSourceChoices)
         && DoseLab::OutputMetadata::IsCanonicalLabel(G4String(row.field),
                                                      DoseLab::OutputMetadata::kFieldChoices)
         && DoseLab::OutputMetadata::IsCanonicalLabel(
           G4String(row.chamber), DoseLab::OutputMetadata::kChamberChoices);
}

bool LoadRunInfoSummaryFromStringSchema(TTree* tree, RunInfoSummary& summary)
{
  if (!tree || tree->GetEntries() <= 0) {
    return false;
  }

  if (!tree->GetBranch("Tag") || !tree->GetBranch("Source") || !tree->GetBranch("Field")
      || !tree->GetBranch(DoseLab::AnalysisConfig::kRunInfoDepthCmColumnName)
      || !tree->GetBranch("Chamber") || !tree->GetBranch("ThreadId")) {
    return false;
  }

  RunInfoRow row;
  auto* tagLeaf = dynamic_cast<TLeafC*>(tree->GetLeaf(DoseLab::AnalysisConfig::kRunInfoTagColumnName));
  auto* sourceLeaf = dynamic_cast<TLeafC*>(tree->GetLeaf(DoseLab::AnalysisConfig::kRunInfoSourceColumnName));
  auto* fieldLeaf = dynamic_cast<TLeafC*>(tree->GetLeaf(DoseLab::AnalysisConfig::kRunInfoFieldColumnName));
  auto* chamberLeaf = dynamic_cast<TLeafC*>(tree->GetLeaf(DoseLab::AnalysisConfig::kRunInfoChamberColumnName));

  if (!tagLeaf || !sourceLeaf || !fieldLeaf || !chamberLeaf) {
    return false;
  }

  tree->SetBranchAddress(DoseLab::AnalysisConfig::kRunInfoDepthCmColumnName, &row.depthCm);
  tree->SetBranchAddress(DoseLab::AnalysisConfig::kRunInfoThreadIdColumnName, &row.threadId);

  const auto entries = tree->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    if (tree->GetEntry(i) <= 0) {
      continue;
    }

    row.tag = tagLeaf->GetValueString();
    row.source = sourceLeaf->GetValueString();
    row.field = fieldLeaf->GetValueString();
    row.chamber = chamberLeaf->GetValueString();

    if (!summary.hasReferenceRow) {
      summary.referenceRow = row;
      summary.hasReferenceRow = true;
    }
    else if (!SameScenarioMetadata(summary.referenceRow, row)) {
      summary.metadataConsistent = false;
    }

    summary.threadIds.insert(row.threadId);
    ++summary.rows;
  }

  tree->ResetBranchAddresses();
  return summary.rows > 0;
}

bool LoadRunInfoSummary(TTree* tree, RunInfoSummary& summary)
{
  return LoadRunInfoSummaryFromStringSchema(tree, summary);
}

bool SummarizeDoubleBranch(TTree* tree, const char* branchName, NumericSummary& summary)
{
  if (!tree) {
    return false;
  }

  auto* branch = tree->GetBranch(branchName);
  if (!branch) {
    return false;
  }

  Double_t value = 0.;
  tree->SetBranchAddress(branchName, &value);

  const auto entries = tree->GetEntries();
  if (entries <= 0) {
    tree->ResetBranchAddresses();
    return true;
  }

  double sum = 0.;
  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::lowest();

  for (Long64_t i = 0; i < entries; ++i) {
    if (branch->GetEntry(i) <= 0) {
      continue;
    }

    const double current = value;
    sum += current;
    if (current < min) {
      min = current;
    }
    if (current > max) {
      max = current;
    }
    ++summary.count;
  }

  tree->ResetBranchAddresses();

  if (summary.count == 0) {
    return true;
  }

  summary.mean = sum / static_cast<double>(summary.count);
  summary.min = min;
  summary.max = max;
  return true;
}

void PrintUsage(const char* exe)
{
  std::cerr << "Usage: " << exe << " <root-file>\n";
  std::cerr << "Example: " << exe << " doseLab-run-ref-10x10-d5cm-6mv-roos.root\n";
}

void PrintNumericBranchSummary(TTree* tree, const char* branchName,
                               std::string (*formatter)(double))
{
  NumericSummary summary;
  if (!SummarizeDoubleBranch(tree, branchName, summary)) {
    std::cout << "  - " << branchName << ": not found\n";
    return;
  }

  if (summary.count <= 0) {
    std::cout << "  - " << branchName << ": no entries\n";
    return;
  }

  std::cout << "  - " << branchName << ": mean=" << formatter(summary.mean)
            << ", min=" << formatter(summary.min)
            << ", max=" << formatter(summary.max) << "\n";
}

void PrintAvailableTrees(TFile* file)
{
  std::cout << "Available TTrees:\n";
  TIter next(file->GetListOfKeys());
  TKey* key = nullptr;
  while ((key = static_cast<TKey*>(next()))) {
    if (std::string(key->GetClassName()) == "TTree") {
      std::cout << "  - " << key->GetName() << "\n";
    }
  }
}

}  // namespace

int main(int argc, char** argv)
{
  if (argc != 2) {
    PrintUsage(argv[0]);
    return EXIT_FAILURE;
  }

  const std::string filePath = argv[1];
  TFile file(filePath.c_str(), "READ");
  if (file.IsZombie()) {
    std::cerr << "Error: failed to open ROOT file: " << filePath << "\n";
    return EXIT_FAILURE;
  }

  std::cout << "\n=== doseLab ROOT Summary ===\n";
  std::cout << "File: " << filePath << "\n\n";

  auto* runInfo = dynamic_cast<TTree*>(file.Get(DoseLab::AnalysisConfig::kRunInfoNtupleName));
  auto* cavity = dynamic_cast<TTree*>(file.Get("cavity"));

  if (!runInfo || !cavity) {
    std::cerr << "Warning: expected ntuple(s) not found.\n";
    PrintAvailableTrees(&file);
  }

  if (runInfo) {
    std::cout << "[runinfo]\n";
    std::cout << "  entries: " << runInfo->GetEntries() << "\n";
    if (runInfo->GetEntries() > 0) {
      std::cout << "  scenario summary:\n";
      RunInfoSummary summary;
      if (LoadRunInfoSummary(runInfo, summary)) {
        const auto& row = summary.referenceRow;
        std::cout << "    Tag            = " << row.tag << "\n";
        std::cout << "    Source         = " << row.source << "\n";
        std::cout << "    Field          = " << row.field << "\n";
        std::cout << "    DepthCm        = " << row.depthCm << "\n";
        std::cout << "    Chamber        = " << row.chamber << "\n";
        std::cout << "    WorkerRows     = " << summary.rows << "\n";
        std::cout << "    WorkerThreads  = " << summary.threadIds.size();
        if (!summary.threadIds.empty()) {
          std::cout << " [" << FormatThreadIdSet(summary.threadIds) << "]";
        }
        std::cout << "\n";
        if (!HasCanonicalMetadataLabels(row)) {
          std::cout << "    Warning        = noncanonical metadata label detected\n";
        }
        if (!summary.metadataConsistent) {
          std::cout << "    Warning        = inconsistent run metadata across worker rows\n";
        }
      }
      else {
        std::cout << "    unavailable\n";
      }
    }
    std::cout << "\n";
  }

  if (cavity) {
    std::cout << "[cavity]\n";
    std::cout << "  entries: " << cavity->GetEntries() << "\n";
    PrintNumericBranchSummary(cavity, "Dose", FormatDose);
    PrintNumericBranchSummary(cavity, "Edep", FormatEnergy);
    PrintNumericBranchSummary(cavity, "TrackL", FormatLength);
    std::cout << "\n";
  }

  return EXIT_SUCCESS;
}
