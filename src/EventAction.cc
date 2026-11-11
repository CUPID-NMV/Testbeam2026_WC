#include "EventAction.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4GenericMessenger.hh"
#include "OrigineFotone.hh"

namespace {
G4bool gOnlyActive = false;
G4bool gPhotonOrigin = false;   // /wc/output/photonOrigin (OrigineFotone.hh)
G4GenericMessenger* gMessenger = nullptr;
}

void EventAction::CreateCommands() {
  if (gMessenger) return;
  gMessenger = new G4GenericMessenger(&gOnlyActive, "/wc/output/", "uscita");
  gMessenger->DeclareProperty("onlyActive", gOnlyActive,
      "true: in \"Eventi\" solo gli eventi con fotoni ai PMT o energia in un pannello "
      "(i muoni sempre). I generati li conta l'ntupla \"Run\". Per le produzioni di gamma.");
  gMessenger->DeclareProperty("photonOrigin", gPhotonOrigin,
      "true: ogni fotone in \"Fotoni\" porta l'origine della sua catena (Origine: 0 Cherenkov in acqua, "
      "1 scintillazione nel nucleo delle fibre del WC, 2 Cherenkov nel nucleo, 3 altro) e, per la "
      "scintillazione, il ritardo di emissione (Ritardo_scint_ns). Di serie false (-1 e 0). Non cambia la fisica.");
}

G4bool OrigineFotone::Attiva() { return gPhotonOrigin; }

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
    fEdepPanel[k] = 0.; fPosPanel[k] = G4ThreeVector(); fTrackLPanel[k] = 0.;
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
  // Con /wc/output/onlyActive: niente riga per gli eventi in cui non succede
  // niente (l'84% dei gamma di Hall A). I fotoni in "Fotoni" ci sono solo
  // negli eventi attivi, che restano.
  if (gOnlyActive && fSourceType != 2) {
    const G4bool attivo = fPMTHits[0] + fPMTHits[1] > 0 ||
                          fEdepPanel[0] + fEdepPanel[1] + fEdepPanel[2] > 0. ||
                          fSiPMTotal[0] + fSiPMTotal[1] + fSiPMTotal[2] > 0;
    if (!attivo) return;
  }

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
  // baricentro dei depositi in ogni pannello, nel sistema dello scintillatore
  // (0 se nel pannello non c'e' energia)
  for (int k = 0; k < 3; k++) {
    const G4ThreeVector b = (fEdepPanel[k] > 0.) ? fPosPanel[k]/fEdepPanel[k] : G4ThreeVector();
    am->FillNtupleDColumn(0, c++, b.x()/mm);
    am->FillNtupleDColumn(0, c++, b.y()/mm);
    am->FillNtupleDColumn(0, c++, b.z()/mm);
  }
  am->AddNtupleRow(0);
}
