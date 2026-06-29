// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabDetectorMessenger.cc
/// \brief UI messenger for DoseLab detector and cavity configuration

#include "DoseLabDetectorMessenger.hh"

#include "DoseLabDetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIdirectory.hh"
#include "globals.hh"

namespace DoseLab
{

DoseLabDetectorMessenger::DoseLabDetectorMessenger(DoseLabDetectorConstruction* detector)
: G4UImessenger(),
  fDetector(detector)
{
  fDoseLabDir = new G4UIdirectory("/doseLab/");
  fDoseLabDir->SetGuidance("doseLab control commands.");

  fCavityDir = new G4UIdirectory("/doseLab/cavity/");
  fCavityDir->SetGuidance("Cavity (ion chamber) geometry commands.");

  fTypeCmd = new G4UIcmdWithAString("/doseLab/cavity/type", this);
  fTypeCmd->SetGuidance(
    "Set cavity type preset: farmer, roos, farmer_walled, roos_walled, custom.");
  fTypeCmd->SetParameterName("type", false);
  fTypeCmd->SetCandidates("farmer roos farmer_walled roos_walled custom");
  fTypeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fRadiusCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/radius", this);
  fRadiusCmd->SetGuidance("Set cavity radius.");
  fRadiusCmd->SetParameterName("radius", false);
  fRadiusCmd->SetUnitCategory("Length");
  fRadiusCmd->SetRange("radius>0.");
  fRadiusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fThicknessCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/thickness", this);
  fThicknessCmd->SetGuidance("Set cavity thickness (along cylinder axis).");
  fThicknessCmd->SetParameterName("thickness", false);
  fThicknessCmd->SetUnitCategory("Length");
  fThicknessCmd->SetRange("thickness>0.");
  fThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fDepthCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/depth", this);
  fDepthCmd->SetGuidance("Set cavity center depth from phantom entrance surface (z=0).");
  fDepthCmd->SetParameterName("depth", false);
  fDepthCmd->SetUnitCategory("Length");
  fDepthCmd->SetRange("depth>0.");
  fDepthCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fAxisCmd = new G4UIcmdWithAString("/doseLab/cavity/axis", this);
  fAxisCmd->SetGuidance("Set cavity axis orientation: x, y, z.");
  fAxisCmd->SetParameterName("axis", false);
  fAxisCmd->SetCandidates("x y z");
  fAxisCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fMaterialCmd = new G4UIcmdWithAString("/doseLab/cavity/material", this);
  fMaterialCmd->SetGuidance("Set cavity material (NIST name, e.g. G4_AIR).");
  fMaterialCmd->SetParameterName("material", false);
  fMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fWallThicknessCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/wallThickness", this);
  fWallThicknessCmd->SetGuidance("Set cavity wall thickness.");
  fWallThicknessCmd->SetParameterName("wallThickness", false);
  fWallThicknessCmd->SetUnitCategory("Length");
  fWallThicknessCmd->SetRange("wallThickness>0.");
  fWallThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fWallMaterialCmd = new G4UIcmdWithAString("/doseLab/cavity/wallMaterial", this);
  fWallMaterialCmd->SetGuidance("Set cavity wall material (NIST name, e.g. G4_PLEXIGLASS).");
  fWallMaterialCmd->SetParameterName("wallMaterial", false);
  fWallMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fCavityCutCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/cut", this);
  fCavityCutCmd->SetGuidance("Set production cut for cavity region.");
  fCavityCutCmd->SetParameterName("cavityCut", false);
  fCavityCutCmd->SetUnitCategory("Length");
  fCavityCutCmd->SetRange("cavityCut>0.");
  fCavityCutCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fWallCutCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/wallCut", this);
  fWallCutCmd->SetGuidance("Set production cut for cavity wall region.");
  fWallCutCmd->SetParameterName("wallCut", false);
  fWallCutCmd->SetUnitCategory("Length");
  fWallCutCmd->SetRange("wallCut>0.");
  fWallCutCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fCavityMaxStepCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/maxStep", this);
  fCavityMaxStepCmd->SetGuidance("Set max step length in cavity region.");
  fCavityMaxStepCmd->SetParameterName("maxStep", false);
  fCavityMaxStepCmd->SetUnitCategory("Length");
  fCavityMaxStepCmd->SetRange("maxStep>0.");
  fCavityMaxStepCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fWallMaxStepCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/wallMaxStep", this);
  fWallMaxStepCmd->SetGuidance("Set max step length in cavity wall region.");
  fWallMaxStepCmd->SetParameterName("wallMaxStep", false);
  fWallMaxStepCmd->SetUnitCategory("Length");
  fWallMaxStepCmd->SetRange("wallMaxStep>0.");
  fWallMaxStepCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fUpdateCmd = new G4UIcmdWithoutParameter("/doseLab/cavity/update", this);
  fUpdateCmd->SetGuidance("Apply command is disabled in Qt interactive mode for stability.");
  fUpdateCmd->SetGuidance("Set cavity parameters before /run/initialize instead.");
  fUpdateCmd->AvailableForStates(G4State_Idle);

  fPrintCmd = new G4UIcmdWithoutParameter("/doseLab/cavity/print", this);
  fPrintCmd->SetGuidance("Print current cavity configuration.");
  fPrintCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

DoseLabDetectorMessenger::~DoseLabDetectorMessenger()
{
  delete fPrintCmd;
  delete fUpdateCmd;
  delete fWallMaxStepCmd;
  delete fCavityMaxStepCmd;
  delete fWallCutCmd;
  delete fCavityCutCmd;
  delete fWallMaterialCmd;
  delete fWallThicknessCmd;
  delete fMaterialCmd;
  delete fAxisCmd;
  delete fDepthCmd;
  delete fThicknessCmd;
  delete fRadiusCmd;
  delete fTypeCmd;
  delete fCavityDir;
  delete fDoseLabDir;
}

void DoseLabDetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  G4bool geometryCommand = false;

  if (command == fTypeCmd) {
    fDetector->SetCavityType(newValue);
    geometryCommand = true;
  }
  else if (command == fRadiusCmd) {
    fDetector->SetCavityRadius(fRadiusCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fThicknessCmd) {
    fDetector->SetCavityThickness(fThicknessCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fDepthCmd) {
    fDetector->SetCavityDepth(fDepthCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fAxisCmd) {
    fDetector->SetCavityAxis(newValue);
    geometryCommand = true;
  }
  else if (command == fMaterialCmd) {
    fDetector->SetCavityMaterial(newValue);
    geometryCommand = true;
  }
  else if (command == fWallThicknessCmd) {
    fDetector->SetCavityWallThickness(fWallThicknessCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fWallMaterialCmd) {
    fDetector->SetCavityWallMaterial(newValue);
    geometryCommand = true;
  }
  else if (command == fCavityCutCmd) {
    fDetector->SetCavityRegionCut(fCavityCutCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fWallCutCmd) {
    fDetector->SetWallRegionCut(fWallCutCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fCavityMaxStepCmd) {
    fDetector->SetCavityMaxStep(fCavityMaxStepCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fWallMaxStepCmd) {
    fDetector->SetWallMaxStep(fWallMaxStepCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fUpdateCmd) {
      G4cout << "Live geometry update is disabled to avoid Qt/ToolsSG scene crashes." << G4endl;
      G4cout << "Please set cavity commands before /run/initialize, then run /control/execute vis.mac."
        << G4endl;
      G4cout << fDetector->GetCavitySummary() << G4endl;
  }
  else if (command == fPrintCmd) {
    G4cout << fDetector->GetCavitySummary() << G4endl;
  }

  if (geometryCommand) {
    G4cout << "Cavity parameters updated. Run /doseLab/cavity/update to apply geometry changes."
           << G4endl;
  }
}

}  // namespace DoseLab
