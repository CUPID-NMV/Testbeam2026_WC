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
  // Riemissione del WLS con decadimento esponenziale (WLSTIMECONSTANT: 2.41 ns
  // nelle fibre del WC, 12 ns in quelle dei pannelli). Il default di Geant4
  // 11.4 e' "delta": fino al 2026-11-03 ogni fotone usciva dopo esattamente la
  // costante di tempo, un ritardo fisso senza coda.
  params->SetWLSTimeProfile("exponential");
  params->SetVerboseLevel(1);
}

PhysicsList::~PhysicsList() {}
