#include "PhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4EmExtraPhysics.hh"
#include "G4OpticalPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4OpticalParameters.hh"

PhysicsList::PhysicsList(): G4VModularPhysicsList() {
  RegisterPhysics(new G4EmStandardPhysics());
  RegisterPhysics(new G4EmExtraPhysics());  // muon-nuclear + gamma-nuclear
  RegisterPhysics(new G4DecayPhysics());

  G4OpticalPhysics* opticalPhysics = new G4OpticalPhysics();
  RegisterPhysics(opticalPhysics);

  G4OpticalParameters* params = G4OpticalParameters::Instance();
  params->SetProcessActivation("Cerenkov",      true);
  // Accesa dal 2026-10-29: scintilla il nucleo delle fibre del WC
  // (FiberScintillation.hh) e, solo con /wc/panel/optics true, l'EJ-200 dei
  // pannelli: gli unici materiali con le proprieta' di scintillazione.
  // /wc/fiber/scintYield 0 riporta il WC al comportamento di prima.
  params->SetProcessActivation("Scintillation", true);
  params->SetProcessActivation("OpAbsorption",  true);
  params->SetProcessActivation("OpRayleigh",    true);
  params->SetProcessActivation("OpBoundary",    true);
  params->SetProcessActivation("OpWLS",         true);
  params->SetVerboseLevel(1);
}

PhysicsList::~PhysicsList() {}
