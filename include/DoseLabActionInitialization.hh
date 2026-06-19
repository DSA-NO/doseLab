#pragma once

#include "G4VUserActionInitialization.hh"

class DoseLabActionInitialization : public G4VUserActionInitialization
{
  public:
    DoseLabActionInitialization();
    ~DoseLabActionInitialization() override;

    void Build() const override;
};