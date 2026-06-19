#include "DoseLabActionInitialization.hh"

#include "DoseLabPrimaryGeneratorAction.hh"

DoseLabActionInitialization::DoseLabActionInitialization() {}
DoseLabActionInitialization::~DoseLabActionInitialization() {}

void DoseLabActionInitialization::Build() const
{
    SetUserAction(new DoseLabPrimaryGeneratorAction());
}