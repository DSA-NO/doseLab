// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file doseLab.cc
/// \brief Main program of the doseLab application

#include "DoseLabActionInitialization.hh"
#include "DoseLabDetectorConstruction.hh"
#include "FTFP_BERT.hh"

#include "G4AnalysisManager.hh"
#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4TScoreNtupleWriter.hh"
#include "G4UIExecutive.hh"
#include "G4UIcommand.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
// #include "Randomize.hh"

namespace
{
void PrintUsage()
{
  G4cerr << " Usage: " << G4endl;
  G4cerr << " doseLab [-m macro ] [-u UIsession] [-t nThreads] [-vDefault]" << G4endl;
  G4cerr << "   note: -t option is available only for multi-threaded mode." << G4endl;
}
}  // namespace

int main(int argc, char** argv)
{
  // Evaluate arguments
  //
  if (argc > 7) {
    PrintUsage();
    return 1;
  }

  G4String macro;
  G4String session;
  G4bool verboseBestUnits = true;
#ifdef G4MULTITHREADED
  G4int nThreads = 0;
#endif
  for (G4int i = 1; i < argc; i = i + 2) {
    if (G4String(argv[i]) == "-m")
      macro = argv[i + 1];
    else if (G4String(argv[i]) == "-u")
      session = argv[i + 1];
#ifdef G4MULTITHREADED
    else if (G4String(argv[i]) == "-t") {
      nThreads = G4UIcommand::ConvertToInt(argv[i + 1]);
    }
#endif
    else if (G4String(argv[i]) == "-vDefault") {
      verboseBestUnits = false;
      --i;  // this option is not followed with a parameter
    }
    else {
      PrintUsage();
      return 1;
    }
  }

  // Detect interactive mode (if no macro provided) and define UI session
  //
  G4UIExecutive* ui = nullptr;
  if (!macro.size()) {
    ui = new G4UIExecutive(argc, argv, session);
  }

  // Optionally: choose a different Random engine...
  // G4Random::setTheEngine(new CLHEP::MTwistEngine);

  // Use G4SteppingVerboseWithUnits
  if (verboseBestUnits) {
    G4int precision = 4;
    G4SteppingVerbose::UseBestUnit(precision);
  }

  // Construct the default run manager
  //
  auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);
#ifdef G4MULTITHREADED
  if (nThreads > 0) {
    runManager->SetNumberOfThreads(nThreads);
  }
#endif

  // Set mandatory initialization classes
  //
  auto detConstruction = new DoseLab::DoseLabDetectorConstruction();
  runManager->SetUserInitialization(detConstruction);

  auto physicsList = new FTFP_BERT;
  runManager->SetUserInitialization(physicsList);

  auto actionInitialization = new DoseLab::DoseLabActionInitialization();
  runManager->SetUserInitialization(actionInitialization);

  // Initialize visualization
  auto visManager = new G4VisExecutive;
  // G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
  // auto visManager = new G4VisExecutive("Quiet");
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  auto UImanager = G4UImanager::GetUIpointer();

  // Activate score ntuple writer
  // The verbose level can be also set via UI commands
  // /score/ntuple/writerVerbose level
  // The default file type ("root") can be changed in xml, csv, hdf5
  // scoreNtupleWriter.SetDefaultFileType("xml");
  G4TScoreNtupleWriter<G4AnalysisManager> scoreNtupleWriter;
  scoreNtupleWriter.SetVerboseLevel(1);
  scoreNtupleWriter.SetNtupleMerging(true);
  // Note: merging ntuples is available only with Root output
  // (the default in G4TScoreNtupleWriter)

  // Process macro or start UI session
  //
  if (macro.size()) {
    // batch mode
    G4String command = "/control/execute ";
    UImanager->ApplyCommand(command + macro);
  }
  else {
    // interactive mode : define UI session
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    if (ui->IsGUI()) {
      UImanager->ApplyCommand("/control/execute gui.mac");
    }
    ui->SessionStart();
    delete ui;
  }

  // Job termination
  // Free the store: user actions, physics_list and detector_description are
  // owned and deleted by the run manager, so they should not be deleted
  // in the main() program !

  delete visManager;
  delete runManager;
}
