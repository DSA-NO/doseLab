// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file doseLab.cc
/// \brief Main program of the doseLab application

#include "DoseLabActionInitialization.hh"
#include "DoseLabDetectorConstruction.hh"
#include "DoseLabMacroRuntime.hh"
#include "FTFP_BERT.hh"
#include "G4EmLivermorePhysics.hh"
#include "G4EmPenelopePhysics.hh"
#include "G4EmStandardPhysics_option4.hh"

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
  G4cerr << " doseLab [-b macro] [-v macro] [-t nThreads] [-p emModel]" << G4endl;
  G4cerr << "   -b macro  : batch mode, no window" << G4endl;
  G4cerr << "   -v macro  : visual mode, opens Qt window, executes macro, stays open" << G4endl;
  G4cerr << "   (no args) : interactive Qt session" << G4endl;
  G4cerr << "   -t N      : set number of threads (multi-threaded build only)" << G4endl;
  G4cerr << "   -p model  : EM model: option4 (default), livermore, penelope" << G4endl;
}
}  // namespace

int main(int argc, char** argv)
{
  // Evaluate arguments
  //
  if (argc > 9) {
    PrintUsage();
    return 1;
  }

  G4String macro;
  G4String visMacro;
  G4String emModel = "option4";
  G4bool verboseBestUnits = true;
#ifdef G4MULTITHREADED
  G4int nThreads = 0;
#endif
  for (G4int i = 1; i < argc; i = i + 2) {
    if (G4String(argv[i]) == "-b")
      macro = argv[i + 1];
    else if (G4String(argv[i]) == "-v")
      visMacro = argv[i + 1];
    else if (G4String(argv[i]) == "-p") {
      emModel = argv[i + 1];
    }
#ifdef G4MULTITHREADED
    else if (G4String(argv[i]) == "-t") {
      nThreads = G4UIcommand::ConvertToInt(argv[i + 1]);
    }
#endif
    else {
      PrintUsage();
      return 1;
    }
  }

  if (macro.size() && visMacro.size()) {
    G4cerr << "Error: -b and -v are mutually exclusive." << G4endl;
    PrintUsage();
    return 1;
  }

  // Create Qt UI session for interactive and visual-macro modes
  //
  G4UIExecutive* ui = nullptr;
  if (!macro.size()) {
    ui = new G4UIExecutive(argc, argv);
  }

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
  if (emModel == "option4") {
    physicsList->ReplacePhysics(new G4EmStandardPhysics_option4());
  }
  else if (emModel == "livermore") {
    physicsList->ReplacePhysics(new G4EmLivermorePhysics());
  }
  else if (emModel == "penelope") {
    physicsList->ReplacePhysics(new G4EmPenelopePhysics());
  }
  else {
    G4cerr << "Error: unknown EM model '" << emModel
           << "'. Use: option4, livermore, penelope" << G4endl;
    PrintUsage();
    delete physicsList;
    delete runManager;
    return 1;
  }
  runManager->SetUserInitialization(physicsList);

  auto actionInitialization = new DoseLab::DoseLabActionInitialization(detConstruction);
  runManager->SetUserInitialization(actionInitialization);

  // Initialize visualization
  auto visManager = new G4VisExecutive;
  // G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
  // auto visManager = new G4VisExecutive("Quiet");
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  auto UImanager = G4UImanager::GetUIpointer();

  // Resolve macro paths from invocation context and move to the macro working
  // directory so nested /control/execute behaves the same in CLI and IDE runs.
  const auto runtimeMacroConfig = DoseLab::MacroRuntime::ResolveRuntimeMacroConfig(argv, macro, visMacro);
  {
    G4String warning;
    if (!DoseLab::MacroRuntime::ApplyWorkingDirectory(runtimeMacroConfig, warning)) {
      G4cerr << "Warning: " << warning << G4endl;
    }
  }

  // Activate score ntuple writer (ROOT output by default).
  // Verbosity can also be adjusted via /score/ntuple/writerVerbose.
  G4TScoreNtupleWriter<G4AnalysisManager> scoreNtupleWriter;
  scoreNtupleWriter.SetVerboseLevel(1);
  scoreNtupleWriter.SetNtupleMerging(true);
  // Ntuple merging is available with ROOT output.

  // Process macro or start UI session
  //
  if (macro.size()) {
    // batch mode: no Qt window
    G4String command = "/control/execute ";
    UImanager->ApplyCommand(command + runtimeMacroConfig.batchMacroArg);
  }
  else if (visMacro.size()) {
    // visual macro mode: Qt window open, execute macro, keep session open for inspection
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    if (ui->IsGUI()) {
      UImanager->ApplyCommand("/control/execute gui.mac");
    }
    UImanager->ApplyCommand("/control/execute " + runtimeMacroConfig.visMacroArg);
    ui->SessionStart();
    delete ui;
  }
  else {
    // interactive mode: no macro, full GUI session
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    if (ui->IsGUI()) {
      UImanager->ApplyCommand("/control/execute gui.mac");
    }
    ui->SessionStart();
    delete ui;
  }

  // User actions, physics list, and detector construction are owned by runManager.
  delete visManager;
  delete runManager;
}
