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

  fGeometryDir = new G4UIdirectory("/doseLab/geometry/");
  fGeometryDir->SetGuidance("DoseLab detector geometry commands.");

  fCavityDir = new G4UIdirectory("/doseLab/cavity/");
  fCavityDir->SetGuidance("Deprecated legacy alias for DoseLab geometry commands.");

  fTypeCmd = new G4UIcmdWithAString("/doseLab/geometry/type", this);
  fTypeCmd->SetGuidance(
    "Set cavity type preset: farmer, roos, farmer-walled, roos-walled, custom.");
  fTypeCmd->SetParameterName("type", false);
  fTypeCmd->SetCandidates("farmer roos farmer-walled roos-walled farmer_walled roos_walled custom");
  fTypeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyTypeCmd = new G4UIcmdWithAString("/doseLab/cavity/type", this);
  fLegacyTypeCmd->SetGuidance("Deprecated alias for /doseLab/geometry/type.");
  fLegacyTypeCmd->SetParameterName("type", false);
  fLegacyTypeCmd->SetCandidates("farmer roos farmer-walled roos-walled farmer_walled roos_walled custom");
  fLegacyTypeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fRadiusCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/geometry/radius", this);
  fRadiusCmd->SetGuidance("Set cavity radius.");
  fRadiusCmd->SetParameterName("radius", false);
  fRadiusCmd->SetUnitCategory("Length");
  fRadiusCmd->SetRange("radius>0.");
  fRadiusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyRadiusCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/radius", this);
  fLegacyRadiusCmd->SetGuidance("Deprecated alias for /doseLab/geometry/radius.");
  fLegacyRadiusCmd->SetParameterName("radius", false);
  fLegacyRadiusCmd->SetUnitCategory("Length");
  fLegacyRadiusCmd->SetRange("radius>0.");
  fLegacyRadiusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fThicknessCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/geometry/thickness", this);
  fThicknessCmd->SetGuidance("Set cavity thickness (along cylinder axis).");
  fThicknessCmd->SetParameterName("thickness", false);
  fThicknessCmd->SetUnitCategory("Length");
  fThicknessCmd->SetRange("thickness>0.");
  fThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyThicknessCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/thickness", this);
  fLegacyThicknessCmd->SetGuidance("Deprecated alias for /doseLab/geometry/thickness.");
  fLegacyThicknessCmd->SetParameterName("thickness", false);
  fLegacyThicknessCmd->SetUnitCategory("Length");
  fLegacyThicknessCmd->SetRange("thickness>0.");
  fLegacyThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fDepthCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/geometry/depth", this);
  fDepthCmd->SetGuidance("Set cavity center depth from phantom entrance surface (z=0).");
  fDepthCmd->SetParameterName("depth", false);
  fDepthCmd->SetUnitCategory("Length");
  fDepthCmd->SetRange("depth>0.");
  fDepthCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyDepthCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/depth", this);
  fLegacyDepthCmd->SetGuidance("Deprecated alias for /doseLab/geometry/depth.");
  fLegacyDepthCmd->SetParameterName("depth", false);
  fLegacyDepthCmd->SetUnitCategory("Length");
  fLegacyDepthCmd->SetRange("depth>0.");
  fLegacyDepthCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fAxisCmd = new G4UIcmdWithAString("/doseLab/geometry/axis", this);
  fAxisCmd->SetGuidance("Set cavity axis orientation: x, y, z.");
  fAxisCmd->SetParameterName("axis", false);
  fAxisCmd->SetCandidates("x y z");
  fAxisCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyAxisCmd = new G4UIcmdWithAString("/doseLab/cavity/axis", this);
  fLegacyAxisCmd->SetGuidance("Deprecated alias for /doseLab/geometry/axis.");
  fLegacyAxisCmd->SetParameterName("axis", false);
  fLegacyAxisCmd->SetCandidates("x y z");
  fLegacyAxisCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fMaterialCmd = new G4UIcmdWithAString("/doseLab/geometry/material", this);
  fMaterialCmd->SetGuidance("Set cavity material (NIST name, e.g. G4_AIR).");
  fMaterialCmd->SetParameterName("material", false);
  fMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyMaterialCmd = new G4UIcmdWithAString("/doseLab/cavity/material", this);
  fLegacyMaterialCmd->SetGuidance("Deprecated alias for /doseLab/geometry/material.");
  fLegacyMaterialCmd->SetParameterName("material", false);
  fLegacyMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fWallThicknessCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/geometry/wallThickness", this);
  fWallThicknessCmd->SetGuidance("Set cavity wall thickness.");
  fWallThicknessCmd->SetParameterName("wallThickness", false);
  fWallThicknessCmd->SetUnitCategory("Length");
  fWallThicknessCmd->SetRange("wallThickness>0.");
  fWallThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyWallThicknessCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/wallThickness", this);
  fLegacyWallThicknessCmd->SetGuidance("Deprecated alias for /doseLab/geometry/wallThickness.");
  fLegacyWallThicknessCmd->SetParameterName("wallThickness", false);
  fLegacyWallThicknessCmd->SetUnitCategory("Length");
  fLegacyWallThicknessCmd->SetRange("wallThickness>0.");
  fLegacyWallThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fWallMaterialCmd = new G4UIcmdWithAString("/doseLab/geometry/wallMaterial", this);
  fWallMaterialCmd->SetGuidance("Set cavity wall material (NIST name, e.g. G4_PLEXIGLASS).");
  fWallMaterialCmd->SetParameterName("wallMaterial", false);
  fWallMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyWallMaterialCmd = new G4UIcmdWithAString("/doseLab/cavity/wallMaterial", this);
  fLegacyWallMaterialCmd->SetGuidance("Deprecated alias for /doseLab/geometry/wallMaterial.");
  fLegacyWallMaterialCmd->SetParameterName("wallMaterial", false);
  fLegacyWallMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fCavityCutCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/geometry/cut", this);
  fCavityCutCmd->SetGuidance("Set production cut for cavity region.");
  fCavityCutCmd->SetParameterName("cavityCut", false);
  fCavityCutCmd->SetUnitCategory("Length");
  fCavityCutCmd->SetRange("cavityCut>0.");
  fCavityCutCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyCavityCutCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/cut", this);
  fLegacyCavityCutCmd->SetGuidance("Deprecated alias for /doseLab/geometry/cut.");
  fLegacyCavityCutCmd->SetParameterName("cavityCut", false);
  fLegacyCavityCutCmd->SetUnitCategory("Length");
  fLegacyCavityCutCmd->SetRange("cavityCut>0.");
  fLegacyCavityCutCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fWallCutCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/geometry/wallCut", this);
  fWallCutCmd->SetGuidance("Set production cut for cavity wall region.");
  fWallCutCmd->SetParameterName("wallCut", false);
  fWallCutCmd->SetUnitCategory("Length");
  fWallCutCmd->SetRange("wallCut>0.");
  fWallCutCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyWallCutCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/wallCut", this);
  fLegacyWallCutCmd->SetGuidance("Deprecated alias for /doseLab/geometry/wallCut.");
  fLegacyWallCutCmd->SetParameterName("wallCut", false);
  fLegacyWallCutCmd->SetUnitCategory("Length");
  fLegacyWallCutCmd->SetRange("wallCut>0.");
  fLegacyWallCutCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fCavityMaxStepCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/geometry/maxStep", this);
  fCavityMaxStepCmd->SetGuidance("Set max step length in cavity region.");
  fCavityMaxStepCmd->SetParameterName("maxStep", false);
  fCavityMaxStepCmd->SetUnitCategory("Length");
  fCavityMaxStepCmd->SetRange("maxStep>0.");
  fCavityMaxStepCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyCavityMaxStepCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/maxStep", this);
  fLegacyCavityMaxStepCmd->SetGuidance("Deprecated alias for /doseLab/geometry/maxStep.");
  fLegacyCavityMaxStepCmd->SetParameterName("maxStep", false);
  fLegacyCavityMaxStepCmd->SetUnitCategory("Length");
  fLegacyCavityMaxStepCmd->SetRange("maxStep>0.");
  fLegacyCavityMaxStepCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fWallMaxStepCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/geometry/wallMaxStep", this);
  fWallMaxStepCmd->SetGuidance("Set max step length in cavity wall region.");
  fWallMaxStepCmd->SetParameterName("wallMaxStep", false);
  fWallMaxStepCmd->SetUnitCategory("Length");
  fWallMaxStepCmd->SetRange("wallMaxStep>0.");
  fWallMaxStepCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyWallMaxStepCmd = new G4UIcmdWithADoubleAndUnit("/doseLab/cavity/wallMaxStep", this);
  fLegacyWallMaxStepCmd->SetGuidance("Deprecated alias for /doseLab/geometry/wallMaxStep.");
  fLegacyWallMaxStepCmd->SetParameterName("wallMaxStep", false);
  fLegacyWallMaxStepCmd->SetUnitCategory("Length");
  fLegacyWallMaxStepCmd->SetRange("wallMaxStep>0.");
  fLegacyWallMaxStepCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fUpdateCmd = new G4UIcmdWithoutParameter("/doseLab/geometry/update", this);
  fUpdateCmd->SetGuidance("Set detector geometry commands before /run/initialize.");
  fUpdateCmd->AvailableForStates(G4State_PreInit);

  fLegacyUpdateCmd = new G4UIcmdWithoutParameter("/doseLab/cavity/update", this);
  fLegacyUpdateCmd->SetGuidance("Deprecated alias for /doseLab/geometry/update.");
  fLegacyUpdateCmd->AvailableForStates(G4State_PreInit);

  fPrintCmd = new G4UIcmdWithoutParameter("/doseLab/geometry/print", this);
  fPrintCmd->SetGuidance("Print current cavity configuration.");
  fPrintCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fLegacyPrintCmd = new G4UIcmdWithoutParameter("/doseLab/cavity/print", this);
  fLegacyPrintCmd->SetGuidance("Deprecated alias for /doseLab/geometry/print.");
  fLegacyPrintCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

DoseLabDetectorMessenger::~DoseLabDetectorMessenger()
{
  delete fLegacyPrintCmd;
  delete fPrintCmd;
  delete fLegacyUpdateCmd;
  delete fUpdateCmd;
  delete fLegacyWallMaxStepCmd;
  delete fWallMaxStepCmd;
  delete fLegacyCavityMaxStepCmd;
  delete fCavityMaxStepCmd;
  delete fLegacyWallCutCmd;
  delete fWallCutCmd;
  delete fLegacyCavityCutCmd;
  delete fCavityCutCmd;
  delete fLegacyWallMaterialCmd;
  delete fWallMaterialCmd;
  delete fLegacyWallThicknessCmd;
  delete fWallThicknessCmd;
  delete fLegacyMaterialCmd;
  delete fMaterialCmd;
  delete fLegacyAxisCmd;
  delete fAxisCmd;
  delete fLegacyDepthCmd;
  delete fDepthCmd;
  delete fLegacyThicknessCmd;
  delete fThicknessCmd;
  delete fLegacyRadiusCmd;
  delete fRadiusCmd;
  delete fLegacyTypeCmd;
  delete fTypeCmd;
  delete fCavityDir;
  delete fGeometryDir;
  delete fDoseLabDir;
}

void DoseLabDetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  G4bool geometryCommand = false;

  if (command == fTypeCmd || command == fLegacyTypeCmd) {
    fDetector->SetCavityType(newValue);
    geometryCommand = true;
  }
  else if (command == fRadiusCmd || command == fLegacyRadiusCmd) {
    fDetector->SetCavityRadius(fRadiusCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fThicknessCmd || command == fLegacyThicknessCmd) {
    fDetector->SetCavityThickness(fThicknessCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fDepthCmd || command == fLegacyDepthCmd) {
    fDetector->SetCavityDepth(fDepthCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fAxisCmd || command == fLegacyAxisCmd) {
    fDetector->SetCavityAxis(newValue);
    geometryCommand = true;
  }
  else if (command == fMaterialCmd || command == fLegacyMaterialCmd) {
    fDetector->SetCavityMaterial(newValue);
    geometryCommand = true;
  }
  else if (command == fWallThicknessCmd || command == fLegacyWallThicknessCmd) {
    fDetector->SetCavityWallThickness(fWallThicknessCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fWallMaterialCmd || command == fLegacyWallMaterialCmd) {
    fDetector->SetCavityWallMaterial(newValue);
    geometryCommand = true;
  }
  else if (command == fCavityCutCmd || command == fLegacyCavityCutCmd) {
    fDetector->SetCavityRegionCut(fCavityCutCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fWallCutCmd || command == fLegacyWallCutCmd) {
    fDetector->SetWallRegionCut(fWallCutCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fCavityMaxStepCmd || command == fLegacyCavityMaxStepCmd) {
    fDetector->SetCavityMaxStep(fCavityMaxStepCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fWallMaxStepCmd || command == fLegacyWallMaxStepCmd) {
    fDetector->SetWallMaxStep(fWallMaxStepCmd->GetNewDoubleValue(newValue));
    geometryCommand = true;
  }
  else if (command == fUpdateCmd || command == fLegacyUpdateCmd) {
      G4cout << "Live geometry update is disabled to avoid Qt/ToolsSG scene crashes." << G4endl;
      G4cout << "Please set geometry commands before /run/initialize, then run /control/execute vis.mac."
        << G4endl;
      G4cout << fDetector->GetCavitySummary() << G4endl;
  }
  else if (command == fPrintCmd || command == fLegacyPrintCmd) {
    G4cout << fDetector->GetCavitySummary() << G4endl;
  }

  if (geometryCommand) {
    G4cout << "Geometry parameter updated. Set all geometry commands before /run/initialize."
           << G4endl;
  }
}

}  // namespace DoseLab
