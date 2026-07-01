// doseLab - Geant4 dose calculation application
// License: http://cern.ch/geant4/license
// Contact: lindbohansen@gmail.com, elisabeth.hansen@dsa.no

#include "DoseLabMacroRuntime.hh"

#include <filesystem>
#include <string>

#ifndef DOSELAB_SOURCE_DIR
#define DOSELAB_SOURCE_DIR ""
#endif

namespace DoseLab
{
namespace MacroRuntime
{
namespace
{
bool IsExistingDirectory(const std::filesystem::path& path)
{
  std::error_code ec;
  return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
}

std::filesystem::path ResolveExecutableDir(char** argv)
{
  if (argv && argv[0] && *argv[0]) {
    std::error_code ec;
    const auto exePath = std::filesystem::weakly_canonical(argv[0], ec);
    if (!ec) {
      return exePath.parent_path();
    }
  }
  return {};
}

std::filesystem::path ChooseRuntimeWorkingDirectory(char** argv,
                                                    const G4String& macro,
                                                    const G4String& visMacro)
{
  const auto current = std::filesystem::current_path();
  const auto exeDir = ResolveExecutableDir(argv);

  auto macroParentIfAny = [](const G4String& macroArg) {
    if (macroArg.empty()) {
      return std::filesystem::path{};
    }
    const std::filesystem::path path{std::string(macroArg)};
    if (!path.has_parent_path()) {
      return std::filesystem::path{};
    }
    return std::filesystem::absolute(path).parent_path();
  };

  const auto batchMacroParent = macroParentIfAny(macro);
  if (!batchMacroParent.empty() && IsExistingDirectory(batchMacroParent)) {
    return batchMacroParent;
  }

  const auto visMacroParent = macroParentIfAny(visMacro);
  if (!visMacroParent.empty() && IsExistingDirectory(visMacroParent)) {
    return visMacroParent;
  }

  if (!exeDir.empty() && IsExistingDirectory(exeDir)
      && std::filesystem::exists(exeDir / "init_vis.mac")) {
    return exeDir;
  }

  if (IsExistingDirectory(current / "macros") || std::filesystem::exists(current / "init_vis.mac")) {
    return current;
  }

  const std::filesystem::path sourceMacros = std::string(DOSELAB_SOURCE_DIR) + "/macros";
  if (IsExistingDirectory(sourceMacros)) {
    return sourceMacros;
  }

  return current;
}

G4String MacroArgForExecution(const G4String& macroArg)
{
  if (macroArg.empty()) {
    return macroArg;
  }
  const std::filesystem::path path{std::string(macroArg)};
  if (path.has_parent_path()) {
    return G4String(path.filename().string());
  }
  return macroArg;
}
}  // namespace

RuntimeMacroConfig ResolveRuntimeMacroConfig(char** argv,
                                             const G4String& macro,
                                             const G4String& visMacro)
{
  RuntimeMacroConfig config;
  config.batchMacroArg = MacroArgForExecution(macro);
  config.visMacroArg = MacroArgForExecution(visMacro);
  config.workingDirectory = G4String(ChooseRuntimeWorkingDirectory(argv, macro, visMacro).string());
  return config;
}

G4bool ApplyWorkingDirectory(const RuntimeMacroConfig& config, G4String& warning)
{
  std::error_code ec;
  std::filesystem::current_path(std::string(config.workingDirectory), ec);
  if (ec) {
    warning = "failed to switch working directory to '" + config.workingDirectory
              + "' for macro resolution.";
    return false;
  }
  warning = "";
  return true;
}

}  // namespace MacroRuntime
}  // namespace DoseLab
