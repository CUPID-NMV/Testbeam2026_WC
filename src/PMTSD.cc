#include "PMTSD.hh"
#include "EventAction.hh"
#include "G4AnalysisManager.hh"
#include "G4EventManager.hh"
#include "G4Step.hh"
#include "G4OpticalPhoton.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"

PMTSD::PMTSD(G4String name) : G4VSensitiveDetector(name) {}
PMTSD::~PMTSD() {}
void PMTSD::Initialize(G4HCofThisEvent*) {}
void PMTSD::EndOfEvent(G4HCofThisEvent*) {}

G4bool PMTSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {

  if (aStep->GetTrack()->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition())
    return false;

  EventAction* eventAction = static_cast<EventAction*>(
      G4EventManager::GetEventManager()->GetUserEventAction());

  if (!eventAction) {
    G4ExceptionDescription msg;
    msg << "Puntatore nullo! EventAction non registrato per il Worker Thread.";
    G4Exception("PMTSD::ProcessHits()", "NullEventAction", FatalException, msg);
    return false;
  }

  G4String name = GetName();

  // Mappatura nome SD → pmtID (0-5)
  // Geometria single:  PMT_SD_L → 0 (L_Bot),  PMT_SD_R → 1 (R_Bot)
  // Geometria triple:  PMT_SD_L_0 → 0, PMT_SD_R_0 → 1
  //                    PMT_SD_L_1 → 2, PMT_SD_R_1 → 3
  //                    PMT_SD_L_2 → 4, PMT_SD_R_2 → 5
  G4int pmtID = -1;
  if      (name == "PMT_SD_L")   pmtID = 0;
  else if (name == "PMT_SD_R")   pmtID = 1;
  else if (name == "PMT_SD_L_0") pmtID = 0;
  else if (name == "PMT_SD_R_0") pmtID = 1;
  else if (name == "PMT_SD_L_1") pmtID = 2;
  else if (name == "PMT_SD_R_1") pmtID = 3;
  else if (name == "PMT_SD_L_2") pmtID = 4;
  else if (name == "PMT_SD_R_2") pmtID = 5;

  if (pmtID < 0) return false;

  G4double hitTime = aStep->GetPostStepPoint()->GetGlobalTime();
  G4double energy  = aStep->GetTrack()->GetKineticEnergy();
  G4int    trackID = aStep->GetTrack()->GetTrackID();
  G4int    evtID   = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();

  eventAction->AddPMTHit(pmtID);
  eventAction->RegisterPMTHit(hitTime);

  auto am = G4AnalysisManager::Instance();
  am->FillNtupleIColumn(1, 0, evtID);
  am->FillNtupleIColumn(1, 1, trackID);
  am->FillNtupleDColumn(1, 2, energy/eV);
  am->FillNtupleDColumn(1, 3, hitTime/ns);
  am->FillNtupleIColumn(1, 4, pmtID);
  am->AddNtupleRow(1);

  aStep->GetTrack()->SetTrackStatus(fStopAndKill);
  return true;
}
