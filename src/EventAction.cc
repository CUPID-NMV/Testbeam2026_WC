#include "EventAction.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"

EventAction::EventAction() : G4UserEventAction(), fSourceType(0) {}
EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event*) {
  fCountedPhotons.clear();
  fCountedPanelPhotons.clear();
  fEinit = 0.; fEdep = 0.; fTrackLength = 0.;
  fEdepFiber[0] = fEdepFiber[1] = fEdepFiber[2] = 0.;
  fNCerenkovProd = 0; fNCerenkovEntrati = 0;
  fPMTHits[0] = fPMTHits[1] = 0;
  fFirstHitTime = 999999.*ns;
  // fSourceType NON va resettato qui: G4RunManager chiama
  // PrimaryGeneratorAction::GeneratePrimaries() (che lo imposta) PRIMA di
  // BeginOfEventAction(), quindi un reset qui lo sovrascriverebbe sempre a 0.
  fNCer_primary = fNCer_compton = fNCer_pair = fNCer_deltaEM = fNCer_nuclear = 0;
  for (int k = 0; k < 3; k++) {
    fEdepPanel[k] = 0.; fTrackLPanel[k] = 0.;
    fNScint[k] = fPhotonsInFiber[k] = fSiPMTotal[k] = fSiPMDetected[k] = 0;
    fFirstSiPM[k] = 999999.*ns;
  }
}

void EventAction::AddCerenkovByProcess(const G4String& proc, G4bool isPrimary) {
  if (isPrimary) {
    fNCer_primary++;
  } else if (proc == "compt" || proc == "phot") {
    fNCer_compton++;
  } else if (proc == "conv") {
    fNCer_pair++;
  } else if (proc == "eIoni" || proc == "muIoni" || proc == "hIoni") {
    fNCer_deltaEM++;
  } else if (proc.find("nucl") != G4String::npos || proc.find("Nucl") != G4String::npos ||
             proc.find("Nuclear") != G4String::npos) {
    fNCer_nuclear++;
  }
  // altri processi EM (eBrem, muBrems, ecc.) non contribuiscono direttamente
  // a Cerenkov in modo classificabile — rimangono fuori dai 5 contatori
}

void EventAction::RegisterPMTHit(G4double time) {
  if (time < fFirstHitTime) fFirstHitTime = time;
}

void EventAction::AddSiPMPhoton(G4int k, G4bool detected, G4double time) {
  if (!Ok(k)) return;
  fSiPMTotal[k]++;
  if (!detected) return;
  fSiPMDetected[k]++;
  if (time < fFirstSiPM[k]) fFirstSiPM[k] = time;
}

void EventAction::EndOfEventAction(const G4Event* event) {
  // Il primario alla generazione
  G4ThreeVector vtx, dir;
  G4double egen = 0.;
  if (event->GetNumberOfPrimaryVertex() > 0) {
    const G4PrimaryVertex* v = event->GetPrimaryVertex(0);
    vtx = v->GetPosition();
    if (v->GetNumberOfParticle() > 0) {
      const G4PrimaryParticle* p = v->GetPrimary(0);
      egen = p->GetKineticEnergy();
      dir  = p->GetMomentumDirection();
    }
  }
  auto tempo = [](G4double t) { return (t > 900000.*ns) ? -1. : t/ns; };

  // Le colonne nell'ordine in cui le crea RunAction
  auto am = G4AnalysisManager::Instance();
  G4int c = 0;
  am->FillNtupleIColumn(0, c++, event->GetEventID());
  am->FillNtupleIColumn(0, c++, fSourceType);
  am->FillNtupleDColumn(0, c++, egen/MeV);
  am->FillNtupleDColumn(0, c++, vtx.x()/mm);
  am->FillNtupleDColumn(0, c++, vtx.y()/mm);
  am->FillNtupleDColumn(0, c++, vtx.z()/mm);
  am->FillNtupleDColumn(0, c++, dir.x());
  am->FillNtupleDColumn(0, c++, dir.y());
  am->FillNtupleDColumn(0, c++, dir.z());
  // WC
  am->FillNtupleDColumn(0, c++, fEinit/MeV);
  am->FillNtupleDColumn(0, c++, fTrackLength/mm);
  am->FillNtupleDColumn(0, c++, fEdep/MeV);
  am->FillNtupleIColumn(0, c++, fNCerenkovProd);
  am->FillNtupleIColumn(0, c++, fNCerenkovEntrati);
  am->FillNtupleIColumn(0, c++, fNCer_primary);
  am->FillNtupleIColumn(0, c++, fNCer_compton);
  am->FillNtupleIColumn(0, c++, fNCer_pair);
  am->FillNtupleIColumn(0, c++, fNCer_deltaEM);
  am->FillNtupleIColumn(0, c++, fNCer_nuclear);
  for (int i = 0; i < 3; i++) am->FillNtupleDColumn(0, c++, fEdepFiber[i]/MeV);
  am->FillNtupleIColumn(0, c++, fPMTHits[0]);
  am->FillNtupleIColumn(0, c++, fPMTHits[1]);
  am->FillNtupleDColumn(0, c++, tempo(fFirstHitTime));
  // pannelli
  for (int k = 0; k < 3; k++) am->FillNtupleDColumn(0, c++, fEdepPanel[k]/MeV);
  for (int k = 0; k < 3; k++) am->FillNtupleDColumn(0, c++, fTrackLPanel[k]/mm);
  for (int k = 0; k < 3; k++) am->FillNtupleIColumn(0, c++, fNScint[k]);
  for (int k = 0; k < 3; k++) am->FillNtupleIColumn(0, c++, fPhotonsInFiber[k]);
  for (int k = 0; k < 3; k++) am->FillNtupleIColumn(0, c++, fSiPMTotal[k]);
  for (int k = 0; k < 3; k++) am->FillNtupleIColumn(0, c++, fSiPMDetected[k]);
  for (int k = 0; k < 3; k++) am->FillNtupleDColumn(0, c++, tempo(fFirstSiPM[k]));
  am->AddNtupleRow(0);
}
