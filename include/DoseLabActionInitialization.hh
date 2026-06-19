#pragma once

#include "G4VUserActionInitialization.hh"

class DoseLabActionInitialization : public G4VUserActionInitialization
{
  public:
    DoseLabActionInitialization();
    virtual ~DoseLabActionInitialization();

    virtual void Build() const;
};