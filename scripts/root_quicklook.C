#include <iostream>
#include <vector>
#include <string>

#include "TBranch.h"
#include "TCanvas.h"
#include "TClass.h"
#include "TFile.h"
#include "TH1.h"
#include "TKey.h"
#include "TLeaf.h"
#include "TObjArray.h"
#include "TROOT.h"
#include "TString.h"
#include "TSystem.h"
#include "TTree.h"

namespace {
bool IsNumericLeaf(TLeaf* leaf) {
  if (!leaf) return false;
  const TString type = leaf->GetTypeName();
  return type == "Char_t" || type == "UChar_t" || type == "Short_t" ||
         type == "UShort_t" || type == "Int_t" || type == "UInt_t" ||
         type == "Long64_t" || type == "ULong64_t" || type == "Float_t" ||
         type == "Double_t";
}

TString Sanitize(const TString& name) {
  TString out = name;
  out.ReplaceAll("/", "_");
  out.ReplaceAll(" ", "_");
  out.ReplaceAll(":", "_");
  out.ReplaceAll(";", "_");
  return out;
}
}  // namespace

void root_quicklook(const char* inputFile = "build-production/doseLab-run-ref-10x10-d5cm-6mv-roos.root",
                    const char* outputDir = "analysis/root_quicklook",
                    int maxTreeBranchesToPlot = 6) {
  gROOT->SetBatch(kTRUE);

  if (gSystem->AccessPathName(inputFile)) {
    std::cerr << "[root_quicklook] Input file not found: " << inputFile << std::endl;
    return;
  }

  gSystem->mkdir(outputDir, kTRUE);

  TFile* f = TFile::Open(inputFile, "READ");
  if (!f || f->IsZombie()) {
    std::cerr << "[root_quicklook] Failed to open file: " << inputFile << std::endl;
    return;
  }

  std::cout << "[root_quicklook] Opened: " << inputFile << std::endl;
  std::cout << "[root_quicklook] Top-level keys:" << std::endl;
  f->ls();

  TIter nextKey(f->GetListOfKeys());
  TKey* key = nullptr;

  while ((key = static_cast<TKey*>(nextKey()))) {
    TObject* obj = key->ReadObj();
    if (!obj) {
      continue;
    }

    const TString objName = obj->GetName();
    const TString className = obj->ClassName();
    std::cout << "[root_quicklook] Object: " << objName << " (" << className << ")" << std::endl;

    if (obj->InheritsFrom(TH1::Class())) {
      auto* h = static_cast<TH1*>(obj);
      TString cName = "c_" + Sanitize(h->GetName());
      TCanvas c(cName, h->GetTitle(), 1000, 700);
      h->SetLineWidth(2);
      h->Draw("hist");
      TString outPath = TString(outputDir) + "/" + Sanitize(h->GetName()) + ".png";
      c.SaveAs(outPath);
      std::cout << "  saved histogram plot: " << outPath << std::endl;
      continue;
    }

    if (obj->InheritsFrom(TTree::Class())) {
      auto* t = static_cast<TTree*>(obj);
      std::cout << "  entries: " << t->GetEntries() << std::endl;
      t->Print();

      int plotted = 0;
      TObjArray* branches = t->GetListOfBranches();
      for (int i = 0; i < branches->GetEntries() && plotted < maxTreeBranchesToPlot; ++i) {
        auto* br = static_cast<TBranch*>(branches->At(i));
        if (!br) continue;

        TObjArray* leaves = br->GetListOfLeaves();
        if (!leaves || leaves->GetEntries() == 0) continue;

        auto* leaf = static_cast<TLeaf*>(leaves->At(0));
        if (!IsNumericLeaf(leaf)) continue;

        TString branchName = br->GetName();
        TString canvasName = "c_" + Sanitize(t->GetName()) + "_" + Sanitize(branchName);
        TCanvas c(canvasName, branchName, 1000, 700);

        TString histName = "h_" + Sanitize(t->GetName()) + "_" + Sanitize(branchName);
        TString drawExpr = branchName + ">>" + histName + "(120)";
        t->Draw(drawExpr, "", "goff");

        auto* hTmp = static_cast<TH1*>(gDirectory->Get(histName));
        if (!hTmp) continue;

        hTmp->SetLineWidth(2);
        hTmp->SetTitle(t->GetName() + TString(" : ") + branchName);
        hTmp->Draw("hist");

        TString outPath = TString(outputDir) + "/" + Sanitize(t->GetName()) + "__" + Sanitize(branchName) + ".png";
        c.SaveAs(outPath);
        std::cout << "  saved branch plot: " << outPath << std::endl;

        ++plotted;
      }

      if (plotted == 0) {
        std::cout << "  no numeric branches found to auto-plot." << std::endl;
      }
    }
  }

  f->Close();
  std::cout << "[root_quicklook] Done. Plots in: " << outputDir << std::endl;
}
