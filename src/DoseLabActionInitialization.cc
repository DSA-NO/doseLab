#include "DoseLabActionInitialization.hh"

#include "DoseLabPrimaryGeneratorAction.hh"

DoseLabActionInitialization::DoseLabActionInitialization() = default;
DoseLabActionInitialization::~DoseLabActionInitialization() = default;

void DoseLabActionInitialization::Build() const
{
    SetUserAction(new DoseLabPrimaryGeneratorAction());
}