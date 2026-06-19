// Main program for doseLab, a Geant4-based application for dose calculations.

#include <memory>

#include "G4RunManagerFactory.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "QGSP_BERT_HP.hh"

#include "DoseLabActionInitialization.hh"
#include "DoseLabDetectorConstruction.hh"

int main(int argc, char** argv)
{
    if (argc > 3) {
        G4cerr << "Usage: " << argv[0] << " [macro] [gdml_file]" << G4endl;
        return 1;
    }

    // -------------------------------
    // UI setup (interactive vs batch)
    // -------------------------------
    std::unique_ptr<G4UIExecutive> ui;
    if (argc == 1) {
        ui = std::make_unique<G4UIExecutive>(argc, argv);
    }

    // -------------------------------
    // Run manager
    // -------------------------------
    // G4RunManagerFactory selects the correct implementation for the build.
    // The number of threads can still be selected at runtime via macros,
    // e.g. /run/numberOfThreads 4 before /run/initialize.
    auto runManager = std::unique_ptr<G4RunManager>(G4RunManagerFactory::CreateRunManager());

    // -------------------------------
    // Mandatory initializations
    // -------------------------------
    G4String gdmlFile = "";
    if (argc > 2) {
        gdmlFile = argv[2];
    }

    runManager->SetUserInitialization(new DoseLabDetectorConstruction(gdmlFile));
    runManager->SetUserInitialization(new QGSP_BERT_HP);
    runManager->SetUserInitialization(new DoseLabActionInitialization());

    // -------------------------------
    // Visualization
    // -------------------------------
    auto visManager = std::make_unique<G4VisExecutive>();
    visManager->Initialize();

    // -------------------------------
    // UI manager
    // -------------------------------
    auto* uiManager = G4UImanager::GetUIpointer();
    uiManager->ApplyCommand("/control/macroPath ./macros");

    // -------------------------------
    // Execution logic
    // -------------------------------
    if (!ui) {
        // Batch mode: run macro from the argument list.
        G4String command = "/control/execute ";
        G4String macro = argv[1];
        uiManager->ApplyCommand(command + macro);
    } else {
        // Interactive mode.
        uiManager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
    }

    return 0;
}
