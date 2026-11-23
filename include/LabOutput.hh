#ifndef LabOutput_h
#define LabOutput_h 1

#include "G4Exception.hh"
#include "globals.hh"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace LabOutput
{
struct State
{
  std::mutex mutex;
  std::filesystem::path root;
  std::filesystem::path runDirectory;
  G4int runId = -1;
};

inline State& GetState()
{
  static State state;
  return state;
}

inline void Initialize(const std::filesystem::path& sourceDirectory)
{
  auto& state = GetState();
  const char* configured = std::getenv("LAB_OUTPUT_DIR");
  std::error_code error;
  std::filesystem::path root;
  if (configured && *configured) {
    root = configured;
  }
  else {
    const auto sourceExists = std::filesystem::is_directory(sourceDirectory, error);
    if (error && error != std::errc::no_such_file_or_directory) {
      G4ExceptionDescription message;
      message << "Cannot inspect source directory '" << sourceDirectory.string() << "': " << error.message();
      G4Exception("LabOutput::Initialize", "LabOutput001", FatalException, message);
      return;
    }
    error.clear();
    root = (sourceExists ? sourceDirectory : std::filesystem::current_path(error)) / "output";
    if (error) {
      G4ExceptionDescription message;
      message << "Cannot resolve startup directory: " << error.message();
      G4Exception("LabOutput::Initialize", "LabOutput001", FatalException, message);
      return;
    }
  }
  state.root = std::filesystem::absolute(root, error);
  if (error) {
    G4ExceptionDescription message;
    message << "Cannot resolve output directory '" << root.string() << "': " << error.message();
    G4Exception("LabOutput::Initialize", "LabOutput001", FatalException, message);
  }
}

inline G4String RunFile(G4bool isMaster, G4int runId, const G4String& fileName)
{
  auto& state = GetState();
  std::lock_guard<std::mutex> lock(state.mutex);
  if (isMaster) {
    std::error_code error;
    std::filesystem::create_directories(state.root, error);
    if (error) {
      G4ExceptionDescription message;
      message << "Cannot create output directory '" << state.root.string() << "': " << error.message();
      G4Exception("LabOutput::RunFile", "LabOutput002", FatalException, message);
      return "";
    }

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
#ifdef _WIN32
    const bool timeOk = gmtime_s(&utc, &now) == 0;
#else
    const bool timeOk = gmtime_r(&now, &utc) != nullptr;
#endif
    if (!timeOk) {
      G4Exception("LabOutput::RunFile", "LabOutput003", FatalException, "Cannot obtain UTC timestamp.");
      return "";
    }
    std::ostringstream timestamp;
    timestamp << std::put_time(&utc, "%Y-%m-%dT%H%M%SZ");
    for (unsigned int suffix = 0;; ++suffix) {
      auto name = timestamp.str();
      if (suffix != 0) name += "-" + std::to_string(suffix);
      const auto directory = state.root / name;
      if (std::filesystem::create_directory(directory, error)) {
        state.runDirectory = directory;
        state.runId = runId;
        break;
      }
      if (error) {
        G4ExceptionDescription message;
        message << "Cannot create run directory '" << directory.string() << "': " << error.message();
        G4Exception("LabOutput::RunFile", "LabOutput004", FatalException, message);
        return "";
      }
    }
  }
  if (state.runId != runId || state.runDirectory.empty()) {
    G4Exception("LabOutput::RunFile", "LabOutput005", FatalException, "Run output directory was not initialized.");
    return "";
  }
  return (state.runDirectory / std::string(fileName)).string();
}
}  // namespace LabOutput

#endif
