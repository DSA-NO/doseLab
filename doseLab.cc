// Main application entry point for doseLab.

#include <memory>

#include "G4PhysListFactory.hh"
#include "G4RunManagerFactory.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

#include "DoseLabActionInitialization.hh"
#include "DoseLabDetectorConstruction.hh"

int main(int argc, char** argv)
{
    if (argc > 3) {
        G4cerr << "Usage: " << argv[0] << " [macro] [gdml_file]" << G4endl;
        return 1;
    }

    bool interactive = (argc == 1);

    std::unique_ptr<G4UIExecutive> ui;
    if (interactive) {
        ui = std::make_unique<G4UIExecutive>(argc, argv);
    }

    auto* runManager = G4RunManagerFactory::CreateRunManager();

    G4String gdmlFile;
    if (argc == 3) {
        gdmlFile = argv[2];
    }

    runManager->SetUserInitialization(new DoseLabDetectorConstruction(gdmlFile));
    runManager->SetUserInitialization(new DoseLabActionInitialization());

    G4PhysListFactory physListFactory;
    auto* physicsList = physListFactory.GetReferencePhysList("QGSP_BERT_HP");

    if (!physicsList) {
        G4Exception("main", "PhysicsListCreation", FatalException,
                    "Failed to create QGSP_BERT_HP physics list.");
    }

    runManager->SetUserInitialization(physicsList);

    G4VisManager* visManager = nullptr;
    if (interactive) {
        visManager = new G4VisExecutive();
        visManager->Initialize();
    }

    auto* uiManager = G4UImanager::GetUIpointer();
    uiManager->ApplyCommand("/control/macroPath ./macros");

    if (!interactive) {
        runManager->Initialize();
        G4String command = "/control/execute ";
        uiManager->ApplyCommand(command + argv[1]);
    } else {
        uiManager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
    }

    delete visManager;
    delete runManager;

    return 0;
}
