// doseLab - ROOT summary helper
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no

#include "TBranch.h"
#include "TCollection.h"
#include "TFile.h"
#include "TKey.h"
#include "TTree.h"

#include "G4SystemOfUnits.hh"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <limits>
#include <sstream>
#include <string>
#include <initializer_list>

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
  Int_t tagHash = 0;
  Int_t sourceCode = 0;
  Int_t fieldCode = 0;
  Double_t depthCm = 0.;
  Int_t chamberCode = 0;
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
  return left.tagHash == right.tagHash && left.sourceCode == right.sourceCode
         && left.fieldCode == right.fieldCode && left.depthCm == right.depthCm
         && left.chamberCode == right.chamberCode;
}

bool LoadRunInfoSummary(TTree* tree, RunInfoSummary& summary)
{
  if (!tree || tree->GetEntries() <= 0) {
    return false;
  }

  if (!tree->GetBranch("TagHash") || !tree->GetBranch("SourceCode") || !tree->GetBranch("FieldCode")
      || !tree->GetBranch("DepthCm") || !tree->GetBranch("ChamberCode")
      || !tree->GetBranch("ThreadId")) {
    return false;
  }

  RunInfoRow row;
  tree->SetBranchAddress("TagHash", &row.tagHash);
  tree->SetBranchAddress("SourceCode", &row.sourceCode);
  tree->SetBranchAddress("FieldCode", &row.fieldCode);
  tree->SetBranchAddress("DepthCm", &row.depthCm);
  tree->SetBranchAddress("ChamberCode", &row.chamberCode);
  tree->SetBranchAddress("ThreadId", &row.threadId);

  const auto entries = tree->GetEntries();
  for (Long64_t i = 0; i < entries; ++i) {
    if (tree->GetEntry(i) <= 0) {
      continue;
    }

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

  auto* runInfo = dynamic_cast<TTree*>(file.Get("runinfo"));
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
        std::cout << "    TagHash        = " << row.tagHash << "\n";
        std::cout << "    SourceCode     = " << row.sourceCode << "\n";
        std::cout << "    FieldCode      = " << row.fieldCode << "\n";
        std::cout << "    DepthCm        = " << row.depthCm << "\n";
        std::cout << "    ChamberCode    = " << row.chamberCode << "\n";
        std::cout << "    WorkerRows     = " << summary.rows << "\n";
        std::cout << "    WorkerThreads  = " << summary.threadIds.size();
        if (!summary.threadIds.empty()) {
          std::cout << " [" << FormatThreadIdSet(summary.threadIds) << "]";
        }
        std::cout << "\n";
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
