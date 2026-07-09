#include "EventAction.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"

EventAction::EventAction() : G4UserEventAction(), fSourceType(0) {}
EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event*) {
  fCountedPhotons.clear();
  fEinit = 0.; fEdep = 0.; fTrackLength = 0.;
  fNCerenkovProd = 0; fNCerenkovEntrati = 0;
  for (int i = 0; i < 6; i++) fPMTHits[i] = 0;
  fFirstHitTime = 999999.*ns;
  // fSourceType NON va resettato qui: G4RunManager chiama
  // PrimaryGeneratorAction::GeneratePrimaries() (che lo imposta) PRIMA di
  // BeginOfEventAction(), quindi un reset qui lo sovrascriverebbe sempre a 0.
  fNCer_primary = fNCer_compton = fNCer_pair = fNCer_deltaEM = fNCer_nuclear = 0;
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

void EventAction::EndOfEventAction(const G4Event*) {
  auto am = G4AnalysisManager::Instance();
  am->FillNtupleDColumn(0, 0, fEinit/MeV);
  am->FillNtupleDColumn(0, 1, fTrackLength/mm);
  am->FillNtupleDColumn(0, 2, fEdep/MeV);
  am->FillNtupleIColumn(0, 3, fNCerenkovProd);
  am->FillNtupleIColumn(0, 4, fNCerenkovEntrati);
  am->FillNtupleDColumn(0, 5, (fFirstHitTime > 900000.*ns) ? -1. : fFirstHitTime/ns);
  for (int i = 0; i < 6; i++) am->FillNtupleIColumn(0, 6+i, fPMTHits[i]);
  am->FillNtupleIColumn(0, 12, fSourceType);
  am->FillNtupleIColumn(0, 13, fNCer_primary);
  am->FillNtupleIColumn(0, 14, fNCer_compton);
  am->FillNtupleIColumn(0, 15, fNCer_pair);
  am->FillNtupleIColumn(0, 16, fNCer_deltaEM);
  am->FillNtupleIColumn(0, 17, fNCer_nuclear);
  am->AddNtupleRow(0);
}
