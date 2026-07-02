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
    G4UIdirectory* fCavityDir;

    G4UIcmdWithAString* fTypeCmd;
    G4UIcmdWithADoubleAndUnit* fRadiusCmd;
    G4UIcmdWithADoubleAndUnit* fThicknessCmd;
    G4UIcmdWithADoubleAndUnit* fDepthCmd;
    G4UIcmdWithAString* fAxisCmd;
    G4UIcmdWithAString* fMaterialCmd;
    G4UIcmdWithADoubleAndUnit* fWallThicknessCmd;
    G4UIcmdWithAString* fWallMaterialCmd;
    G4UIcmdWithADoubleAndUnit* fCavityCutCmd;
    G4UIcmdWithADoubleAndUnit* fWallCutCmd;
    G4UIcmdWithADoubleAndUnit* fCavityMaxStepCmd;
    G4UIcmdWithADoubleAndUnit* fWallMaxStepCmd;
    G4UIcmdWithoutParameter* fUpdateCmd;  // Deprecated no-op retained for backward compatibility.
    G4UIcmdWithoutParameter* fPrintCmd;
};

}  // namespace DoseLab

#endif
