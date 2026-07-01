// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no

#ifndef DoseLabMacroRuntime_h
#define DoseLabMacroRuntime_h 1

#include "globals.hh"

namespace DoseLab
{
namespace MacroRuntime
{

struct RuntimeMacroConfig
{
  G4String batchMacroArg;
  G4String visMacroArg;
  G4String workingDirectory;
};

RuntimeMacroConfig ResolveRuntimeMacroConfig(char** argv,
                                             const G4String& macro,
                                             const G4String& visMacro);

G4bool ApplyWorkingDirectory(const RuntimeMacroConfig& config, G4String& warning);

}  // namespace MacroRuntime
}  // namespace DoseLab

#endif
