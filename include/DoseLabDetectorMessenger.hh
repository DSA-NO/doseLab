// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no
//
/// \file DoseLabDetectorMessenger.hh
/// \brief UI messenger for DoseLab detector and cavity configuration

#ifndef DoseLabDetectorMessenger_h
#define DoseLabDetectorMessenger_h 1

#include "G4UImessenger.hh"

class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithoutParameter;
class G4UIcommand;

namespace DoseLab
{

class DoseLabDetectorConstruction;

class DoseLabDetectorMessenger : public G4UImessenger
{
  public:
    explicit DoseLabDetectorMessenger(DoseLabDetectorConstruction* detector);
    ~DoseLabDetectorMessenger() override;

    void SetNewValue(G4UIcommand* command, G4String newValue) override;

  private:
    DoseLabDetectorConstruction* fDetector;

    G4UIdirectory* fDoseLabDir;
    G4UIdirectory* fGeometryDir;
    G4UIdirectory* fCavityDir;

    G4UIcmdWithAString* fTypeCmd;
    G4UIcmdWithAString* fLegacyTypeCmd;
    G4UIcmdWithADoubleAndUnit* fRadiusCmd;
    G4UIcmdWithADoubleAndUnit* fLegacyRadiusCmd;
    G4UIcmdWithADoubleAndUnit* fThicknessCmd;
    G4UIcmdWithADoubleAndUnit* fLegacyThicknessCmd;
    G4UIcmdWithADoubleAndUnit* fDepthCmd;
    G4UIcmdWithADoubleAndUnit* fLegacyDepthCmd;
    G4UIcmdWithAString* fAxisCmd;
    G4UIcmdWithAString* fLegacyAxisCmd;
    G4UIcmdWithAString* fMaterialCmd;
    G4UIcmdWithAString* fLegacyMaterialCmd;
    G4UIcmdWithADoubleAndUnit* fWallThicknessCmd;
    G4UIcmdWithADoubleAndUnit* fLegacyWallThicknessCmd;
    G4UIcmdWithAString* fWallMaterialCmd;
    G4UIcmdWithAString* fLegacyWallMaterialCmd;
    G4UIcmdWithADoubleAndUnit* fCavityCutCmd;
    G4UIcmdWithADoubleAndUnit* fLegacyCavityCutCmd;
    G4UIcmdWithADoubleAndUnit* fWallCutCmd;
    G4UIcmdWithADoubleAndUnit* fLegacyWallCutCmd;
    G4UIcmdWithADoubleAndUnit* fCavityMaxStepCmd;
    G4UIcmdWithADoubleAndUnit* fLegacyCavityMaxStepCmd;
    G4UIcmdWithADoubleAndUnit* fWallMaxStepCmd;
    G4UIcmdWithADoubleAndUnit* fLegacyWallMaxStepCmd;
    G4UIcmdWithoutParameter* fUpdateCmd;  // Deprecated no-op retained for backward compatibility.
    G4UIcmdWithoutParameter* fLegacyUpdateCmd;
    G4UIcmdWithoutParameter* fPrintCmd;
    G4UIcmdWithoutParameter* fLegacyPrintCmd;
};

}  // namespace DoseLab

#endif
