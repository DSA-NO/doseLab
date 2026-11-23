#include "G4RootAnalysisReader.hh"
#include "Randomize.hh"

#include <iostream>
#include <set>
#include <sstream>
#include <string>

int main(int argc, char** argv)
{
  if (argc != 3) return 1;
  const int expectedRows = std::stoi(argv[2]);
  auto* reader = G4RootAnalysisReader::Instance();
  const auto id = reader->GetNtuple("runinfo", argv[1]);
  G4String state, engine;
  G4int threadId = 0;
  if (id < 0 || !reader->SetNtupleSColumn(id, "RandomState", state)
      || !reader->SetNtupleSColumn(id, "RandomEngine", engine)
      || !reader->SetNtupleIColumn(id, "ThreadId", threadId)) {
    std::cerr << "Cannot read random state metadata\n";
    return 1;
  }
  const long seeds[] = {12345, 67890, 0};
  G4Random::setTheSeeds(seeds);
  std::ostringstream initialState;
  G4Random::saveFullState(initialState);
  std::set<G4int> threads;
  while (reader->GetNtupleRow(id)) {
    if (engine != G4Random::getTheEngine()->name() || state.empty()
        || !threads.insert(threadId).second) {
      std::cerr << "Invalid runinfo metadata\n";
      return 1;
    }
    if (expectedRows == 1 && state != initialState.str()) {
      std::cerr << "Serial state does not match the state set before the run\n";
      return 1;
    }
    std::istringstream input(state);
    G4Random::restoreFullState(input);
    std::ostringstream restored;
    G4Random::saveFullState(restored);
    if (input.fail() || restored.str() != state) {
      std::cerr << "Random state cannot be restored exactly\n";
      return 1;
    }
  }
  if (threads.size() != static_cast<std::size_t>(expectedRows)) {
    std::cerr << "Unexpected number of runinfo rows\n";
    return 1;
  }
  return 0;
}
