// doseLab - ROOT summary helper
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no

#include "TBranch.h"
#include "TFile.h"
#include "TIter.h"
#include "TKey.h"
#include "TTree.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace
{

void PrintUsage(const char* exe)
{
  std::cerr << "Usage: " << exe << " <root-file>\n";
  std::cerr << "Example: " << exe << " doseLab-run-ref-10x10-d5cm-6mv-roos.root\n";
}

void PrintNumericBranchSummary(TTree* tree, const char* branchName)
{
  if (!tree || !tree->GetBranch(branchName)) {
    std::cout << "  - " << branchName << ": not found\n";
    return;
  }

  const double mean = tree->GetMean(branchName);
  const double min = tree->GetMinimum(branchName);
  const double max = tree->GetMaximum(branchName);

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "  - " << branchName << ": mean=" << mean << ", min=" << min
            << ", max=" << max << "\n";
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
      std::cout << "  first row:\n";
      runInfo->Show(0);
    }
    std::cout << "\n";
  }

  if (cavity) {
    std::cout << "[cavity]\n";
    std::cout << "  entries: " << cavity->GetEntries() << "\n";
    PrintNumericBranchSummary(cavity, "Dose");
    PrintNumericBranchSummary(cavity, "Edep");
    PrintNumericBranchSummary(cavity, "TrackL");
    std::cout << "\n";
  }

  return EXIT_SUCCESS;
}
