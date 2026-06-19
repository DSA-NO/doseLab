#pragma once

#include "G4VUserActionInitialization.hh"

class DoseLabActionInitialization : public G4VUserActionInitialization
{
  public:
    DoseLabActionInitialization() = default;
    ~DoseLabActionInitialization() override = default;

    void Build() const override;
    void BuildForMaster() const override;  // optional but recommended
};
