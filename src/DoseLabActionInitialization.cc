#include "DoseLabActionInitialization.hh"

#include "DoseLabPrimaryGeneratorAction.hh"

void DoseLabActionInitialization::Build() const
{
    SetUserAction(new DoseLabPrimaryGeneratorAction());
}

void DoseLabActionInitialization::BuildForMaster() const
{
}
