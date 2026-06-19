#include "DoseLabActionInitialization.hh"

#include "DoseLabPrimaryGeneratorAction.hh"
#include "DoseLabRunAction.hh"

void DoseLabActionInitialization::Build() const
{
    SetUserAction(new DoseLabPrimaryGeneratorAction());
    SetUserAction(new DoseLabRunAction());
}

void DoseLabActionInitialization::BuildForMaster() const
{
    SetUserAction(new DoseLabRunAction());
}
