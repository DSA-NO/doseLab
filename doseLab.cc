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
#include "G4RadioactiveDecayPhysics.hh"

#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
// #include "Randomize.hh"

#include <array>
#include <cstdlib>
#include <fstream>

namespace
{
void PrintUsage()
{
  G4cerr << " Usage: " << G4endl;
  G4cerr << " doseLab [-b macro] [-v macro] [-t nThreads] [-p emModel] [-r on|off]" << G4endl;
  G4cerr << "   -b macro  : batch mode, execute the given macro" << G4endl;
  G4cerr << "   -v macro  : visualize, execute the macro, and keep the UI open" << G4endl;
  G4cerr << "   (no args) : interactive Qt session" << G4endl;
  G4cerr << "   -t N      : set number of threads (multi-threaded build only)" << G4endl;
  G4cerr << "   -p model  : EM model: option4 (default), livermore, penelope" << G4endl;
  G4cerr << "   -r mode   : radioactive decay physics: off (default), on" << G4endl;
}

bool FileExists(const G4String& path)
{
  std::ifstream f(path);
  return f.good();
}

bool ExecuteMacroWithFallback(G4UImanager* uiManager, const G4String& macro)
{
  const std::array<G4String, 3> candidates = {
    macro,
    "macros/" + macro,
    "build/" + macro,
  };

  for (const auto& candidate : candidates) {
    if (!FileExists(candidate)) {
      continue;
    }
    const auto status = uiManager->ApplyCommand("/control/execute " + candidate);
    if (status == 0) {
      return true;
    }
    G4cerr << "Error: macro '" << candidate << "' failed with UI status " << status << "." << G4endl;
    return false;
  }

  G4cerr << "Error: could not locate macro '" << macro
         << "' in current directory, macros/, or build/." << G4endl;
  return false;
}

FTFP_BERT* CreatePhysicsList(const G4String& emModel, G4bool enableRadioactiveDecay)
{
  auto* physicsList = new FTFP_BERT;
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
    return nullptr;
  }

  if (enableRadioactiveDecay) {
    physicsList->RegisterPhysics(new G4RadioactiveDecayPhysics());
  }

  return physicsList;
}
}  // namespace

int main(int argc, char** argv)
{
  G4String macro;
  G4String visMacro;
  G4String emModel = "option4";
  G4bool enableRadioactiveDecay = false;
  G4bool verboseBestUnits = true;
#ifdef G4MULTITHREADED
  G4int nThreads = 0;
#endif
  for (G4int i = 1; i < argc; ++i) {
    const G4String arg = argv[i];
    if (arg == "-b") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      macro = argv[++i];
    }
    else if (arg == "-v") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      visMacro = argv[++i];
    }
    else if (arg == "-p") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      emModel = argv[++i];
    }
    else if (arg == "-r") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      const G4String decayMode = argv[++i];
      if (decayMode == "on") {
        enableRadioactiveDecay = true;
      }
      else if (decayMode == "off") {
        enableRadioactiveDecay = false;
      }
      else {
        G4cerr << "Error: unknown radioactive decay mode '" << decayMode
               << "'. Use: on, off" << G4endl;
        PrintUsage();
        return 1;
      }
    }
#ifdef G4MULTITHREADED
    else if (arg == "-t") {
      if (i + 1 >= argc) {
        PrintUsage();
        return 1;
      }
      nThreads = std::atoi(argv[++i]);
    }
#endif
    else {
      PrintUsage();
      return 1;
    }
  }

  if (!macro.empty() && !visMacro.empty()) {
    G4cerr << "Error: -b and -v are mutually exclusive." << G4endl;
    PrintUsage();
    return 1;
  }

  // Create Qt UI session for interactive and visual-macro modes
  //
  G4UIExecutive* ui = nullptr;
  if (macro.empty()) {
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

  auto* physicsList = CreatePhysicsList(emModel, enableRadioactiveDecay);
  if (!physicsList) {
    PrintUsage();
    delete runManager;
    return 1;
  }
  runManager->SetUserInitialization(physicsList);

  auto actionInitialization = new DoseLab::DoseLabActionInitialization(detConstruction, emModel, enableRadioactiveDecay);
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

  // Process macro or start UI session
  //
  if (!macro.empty()) {
    if (!ExecuteMacroWithFallback(UImanager, runtimeMacroConfig.batchMacroArg)) {
      delete ui;
      delete visManager;
      delete runManager;
      return 1;
    }
  }
  else if (!visMacro.empty()) {
    // visual macro mode: Qt window open, execute macro, keep session open for inspection
    if (!ExecuteMacroWithFallback(UImanager, "init_vis.mac")) {
      delete ui;
      delete visManager;
      delete runManager;
      return 1;
    }
    if (ui && ui->IsGUI()) {
      if (!ExecuteMacroWithFallback(UImanager, "gui.mac")) {
        delete ui;
        delete visManager;
        delete runManager;
        return 1;
      }
    }
    if (!ExecuteMacroWithFallback(UImanager, runtimeMacroConfig.visMacroArg)) {
      delete ui;
      delete visManager;
      delete runManager;
      return 1;
    }
    ui->SessionStart();
    delete ui;
  }
  else {
    // interactive mode: no macro, full GUI session
    if (!ExecuteMacroWithFallback(UImanager, "init_vis.mac")) {
      delete ui;
      delete visManager;
      delete runManager;
      return 1;
    }
    if (ui && ui->IsGUI()) {
      if (!ExecuteMacroWithFallback(UImanager, "gui.mac")) {
        delete ui;
        delete visManager;
        delete runManager;
        return 1;
      }
    }
    ui->SessionStart();
    delete ui;
  }

  // User actions, physics list, and detector construction are owned by runManager.
  delete visManager;
  delete runManager;
}
