// Main program for doseLab, a Geant4-based application for dose calculations.

#include "G4MTRunManager.hh"
#include "G4RunManager.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "QGSP_BERT_HP.hh"

#include "DoseLabActionInitialization.hh"
#include "DoseLabDetectorConstruction.hh"

int main(int argc, char** argv)
{
    // -------------------------------
    // UI setup (interactive vs batch)
    // -------------------------------
    G4UIExecutive* ui = nullptr;

    if (argc == 1) {
        ui = new G4UIExecutive(argc, argv);
    }

    // -------------------------------
    // Run manager (MT ready)
    // -------------------------------
#ifdef G4MULTITHREADED
    auto runManager = new G4MTRunManager();
    runManager->SetNumberOfThreads(10);  // adjust later
#else
    auto runManager = new G4RunManager();
#endif

    // -------------------------------
    // Mandatory initializations
    // -------------------------------
    // Geometry (optional GDML file can be passed as the second argument)
    G4String gdmlFile = "";
    if (argc > 2) {
        gdmlFile = argv[2];
    }
    runManager->SetUserInitialization(new DoseLabDetectorConstruction(gdmlFile));

    // Physics list
    runManager->SetUserInitialization(new QGSP_BERT_HP);

    // Actions (generator, etc.)
    runManager->SetUserInitialization(new DoseLabActionInitialization());

    // -------------------------------
    // Visualization
    // -------------------------------
    auto visManager = new G4VisExecutive();
    visManager->Initialize();

    // -------------------------------
    // UI manager
    // -------------------------------
    auto uiManager = G4UImanager::GetUIpointer();

    // Make sure the macro directory is visible to Geant4.
    uiManager->ApplyCommand("/control/macroPath ./macros");

    // -------------------------------
    // Execution logic
    // -------------------------------
    if (!ui) {
        // Batch mode: run macro from the argument list
        G4String command = "/control/execute ";
        G4String macro = argv[1];
        uiManager->ApplyCommand(command + macro);
    } else {
        // Interactive mode
        uiManager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
        delete ui;
    }

    // -------------------------------
    // Cleanup
    // -------------------------------
    delete visManager;
    delete runManager;

    return 0;
}
