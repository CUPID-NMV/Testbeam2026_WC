#include "PrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "G4GeneralParticleSource.hh"
#include "G4EventManager.hh"
#include "G4Event.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
: G4VUserPrimaryGeneratorAction(), fGPS(nullptr)
{
  fGPS = new G4GeneralParticleSource();
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() { delete fGPS; }

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  // Determina il tipo di sorgente dalla particella corrente del GPS
  // e lo passa all'EventAction PRIMA di generare il vertice,
  // così è disponibile per tutta la durata dell'evento.
  G4int srcType = 0; // default: e-
  auto* src = fGPS->GetCurrentSource();
  if (src && src->GetParticleDefinition()) {
    G4String pname = src->GetParticleDefinition()->GetParticleName();
    if      (pname == "e-"  || pname == "e+")           srcType = 0;
    else if (pname == "gamma")                           srcType = 1;
    else if (pname == "mu-" || pname == "mu+")           srcType = 2;
  }

  auto* ea = static_cast<EventAction*>(
      G4EventManager::GetEventManager()->GetUserEventAction());
  if (ea) ea->SetSourceType(srcType);

  fGPS->GeneratePrimaryVertex(anEvent);
}
